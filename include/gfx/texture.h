#ifndef GFX_TEXTURE_DECL
#define GFX_TEXTURE_DECL

#include <gfx/core.h>
#include <webgpu/webgpu.h>

// Texture, RGBA8 sampled with its own filter and wrap. Linear textures get
// mipmaps, nearest ones (pixel art, atlases) don't.

enum GFX_Wrap {
  GFX_WRAP_REPEAT,
  GFX_WRAP_MIRROR_REPEAT,
  GFX_WRAP_CLAMP_EDGE,
};

enum GFX_TextureFilter {
  GFX_TEXTURE_FILTER_LINEAR,
  GFX_TEXTURE_FILTER_NEAREST,
};

typedef struct {
  int width;
  int height;
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUSampler sampler;
} GFX_Texture2D;

// Pixels are RGBA8, rows from top to bottom
GFX_Texture2D gfx_texture2d_create(int width, int height, const uint8_t *pixels,
                                   enum GFX_TextureFilter filter,
                                   enum GFX_Wrap wrap);
// On failure the texture is zeroed, materials then use white
GFX_Texture2D gfx_texture2d_load(const char *filename,
                                 enum GFX_TextureFilter filter,
                                 enum GFX_Wrap wrap);
void gfx_texture2d_destroy(const GFX_Texture2D *texture);

#endif
