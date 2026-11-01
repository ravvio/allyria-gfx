#include "gfx_internal.h"
#include <gfx/texture.h>
#include <stb_image.h>
#include <stdio.h>
#include <stdlib.h>

// Number of mip levels down to 1x1
uint32_t gfx_texture_mip_count(int width, int height) {
  int size = width > height ? width : height;
  uint32_t count = 1;
  while (size > 1) {
    size /= 2;
    count++;
  }
  return count;
}

// Halve an RGBA8 image into dst
void gfx_texture_downsample(const uint8_t *src, int src_w, int src_h,
                            uint8_t *dst, int dst_w, int dst_h) {
  for (int y = 0; y < dst_h; y++) {
    const uint8_t *row0 = src + (size_t)(2 * y) * src_w * 4;
    const uint8_t *row1 =
        src + (size_t)(2 * y + 1 < src_h ? 2 * y + 1 : src_h - 1) * src_w * 4;
    for (int x = 0; x < dst_w; x++) {
      int x0 = 2 * x * 4;
      int x1 = (2 * x + 1 < src_w ? 2 * x + 1 : src_w - 1) * 4;
      for (int c = 0; c < 4; c++) {
        int sum = row0[x0 + c] + row0[x1 + c] + row1[x0 + c] + row1[x1 + c];
        dst[((size_t)y * dst_w + x) * 4 + c] = (uint8_t)((sum + 2) / 4);
      }
    }
  }
}

static void _gfx_texture_write(WGPUTexture texture, uint32_t level, int width,
                               int height, const uint8_t *pixels) {
  WGPUTexelCopyTextureInfo destination = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
  destination.texture = texture;
  destination.mipLevel = level;
  WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
  layout.bytesPerRow = width * 4;
  layout.rowsPerImage = height;
  WGPUExtent3D size = {width, height, 1};
  wgpuQueueWriteTexture(_gfx.queue, &destination, pixels,
                        (size_t)width * height * 4, &layout, &size);
}

static WGPUAddressMode _gfx_address_mode(enum GFX_Wrap wrap) {
  switch (wrap) {
  case GFX_WRAP_MIRROR_REPEAT:
    return WGPUAddressMode_MirrorRepeat;
  case GFX_WRAP_CLAMP_EDGE:
    return WGPUAddressMode_ClampToEdge;
  default:
    return WGPUAddressMode_Repeat;
  }
}

// Create a texture from RGBA8 pixels
GFX_Texture2D gfx_texture2d_create(int width, int height, const uint8_t *pixels,
                                   enum GFX_TextureFilter filter,
                                   enum GFX_Wrap wrap) {
  uint32_t mip_count = filter == GFX_TEXTURE_FILTER_LINEAR
                           ? gfx_texture_mip_count(width, height)
                           : 1;

  // Two buffers for the mips, each one is built from the other
  uint8_t *scratch[2] = {NULL, NULL};
  if (mip_count > 1) {
    size_t size =
        (size_t)(width > 1 ? width / 2 : 1) * (height > 1 ? height / 2 : 1) * 4;
    scratch[0] = malloc(size);
    scratch[1] = malloc(size);
    if (!scratch[0] || !scratch[1]) {
      fprintf(stderr, "[e] no memory for mipmaps, texture has one level\n");
      mip_count = 1;
    }
  }

  WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
  desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
  desc.dimension = WGPUTextureDimension_2D;
  desc.size = (WGPUExtent3D){width, height, 1};
  desc.format = WGPUTextureFormat_RGBA8Unorm;
  desc.mipLevelCount = mip_count;
  WGPUTexture texture = wgpuDeviceCreateTexture(_gfx.device, &desc);

  // Upload the pixels, each mip is downsampled from the previous one
  _gfx_texture_write(texture, 0, width, height, pixels);
  const uint8_t *src = pixels;
  int w = width, h = height;
  for (uint32_t level = 1; level < mip_count; level++) {
    uint8_t *dst = scratch[level % 2];
    int dst_w = w > 1 ? w / 2 : 1;
    int dst_h = h > 1 ? h / 2 : 1;
    gfx_texture_downsample(src, w, h, dst, dst_w, dst_h);
    _gfx_texture_write(texture, level, dst_w, dst_h, dst);
    src = dst;
    w = dst_w;
    h = dst_h;
  }
  free(scratch[0]);
  free(scratch[1]);

  // Sampler
  WGPUFilterMode mode = filter == GFX_TEXTURE_FILTER_NEAREST
                            ? WGPUFilterMode_Nearest
                            : WGPUFilterMode_Linear;
  WGPUSamplerDescriptor sampler_desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
  sampler_desc.addressModeU = _gfx_address_mode(wrap);
  sampler_desc.addressModeV = _gfx_address_mode(wrap);
  sampler_desc.magFilter = mode;
  sampler_desc.minFilter = mode;
  sampler_desc.mipmapFilter = mip_count > 1 ? WGPUMipmapFilterMode_Linear
                                            : WGPUMipmapFilterMode_Nearest;

  GFX_Texture2D tex = {
      .width = width,
      .height = height,
      .texture = texture,
      .view = wgpuTextureCreateView(texture, NULL),
      .sampler = wgpuDeviceCreateSampler(_gfx.device, &sampler_desc),
  };
  return tex;
}

// Load a texture from an image file
GFX_Texture2D gfx_texture2d_load(const char *filename,
                                 enum GFX_TextureFilter filter,
                                 enum GFX_Wrap wrap) {
  // Get image data
  int width, height, channels;
  uint8_t *data = stbi_load(filename, &width, &height, &channels, 4);
  if (!data) {
    fprintf(stderr, "[e] failed to load texture image: %s\n| %s\n", filename,
            stbi_failure_reason());
    GFX_Texture2D empty = {0};
    return empty;
  }

  GFX_Texture2D tex = gfx_texture2d_create(width, height, data, filter, wrap);
  stbi_image_free(data);

  fprintf(stderr, "[i] loaded texture %s\n", filename);
  return tex;
}

void gfx_texture2d_destroy(const GFX_Texture2D *texture) {
  if (texture->texture) {
    wgpuSamplerRelease(texture->sampler);
    wgpuTextureViewRelease(texture->view);
    wgpuTextureRelease(texture->texture);
  }
}
