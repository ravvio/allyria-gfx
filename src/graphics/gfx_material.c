// Materials, bind group 2 laid out by a GFX_MaterialLayout:
// binding 0 uniform, then each texture i at 1+2i and its sampler at 2+2i

#include "../../include/gfx/graphics/gfx_wgpu.h"
#include <assert.h>
#include <string.h>

// Used for missing textures
static GFX_Texture2D _white;

// WGSL uniform structs are sized in multiples of 16 bytes
static uint32_t _gfx_uniform_buffer_size(uint32_t uniform_size) {
  return (uniform_size + 15) & ~15u;
}

// ============================================================================
// Material Layout
// ============================================================================

GFX_MaterialLayout gfx_material_layout_create(uint32_t uniform_size,
                                              uint32_t texture_count) {
  assert(uniform_size % 4 == 0);
  assert(texture_count <= GFX_MATERIAL_MAX_TEXTURES);

  WGPUBindGroupLayoutEntry entries[1 + 2 * GFX_MATERIAL_MAX_TEXTURES];
  size_t count = 0;

  if (uniform_size > 0) {
    entries[count] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entries[count].binding = 0;
    entries[count].visibility =
        WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    entries[count].buffer.type = WGPUBufferBindingType_Uniform;
    entries[count].buffer.minBindingSize =
        _gfx_uniform_buffer_size(uniform_size);
    count++;
  }

  for (uint32_t i = 0; i < texture_count; i++) {
    entries[count] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entries[count].binding = 1 + 2 * i;
    entries[count].visibility = WGPUShaderStage_Fragment;
    entries[count].texture.sampleType = WGPUTextureSampleType_Float;
    entries[count].texture.viewDimension = WGPUTextureViewDimension_2D;
    count++;

    entries[count] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entries[count].binding = 2 + 2 * i;
    entries[count].visibility = WGPUShaderStage_Fragment;
    entries[count].sampler.type = WGPUSamplerBindingType_Filtering;
    count++;
  }

  WGPUBindGroupLayoutDescriptor desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  desc.entryCount = count;
  desc.entries = entries;

  GFX_MaterialLayout layout = {
      .uniform_size = uniform_size,
      .texture_count = texture_count,
      .layout = wgpuDeviceCreateBindGroupLayout(_gfx.device, &desc),
  };
  layout.pipeline_layout = gfx_uniforms_pipeline_layout(layout.layout);
  return layout;
}

void gfx_material_layout_destroy(const GFX_MaterialLayout *layout) {
  wgpuPipelineLayoutRelease(layout->pipeline_layout);
  wgpuBindGroupLayoutRelease(layout->layout);
}

// ============================================================================
// Material
// ============================================================================

GFX_Material gfx_material_create(const GFX_MaterialLayout *layout,
                                 const void *uniform_data,
                                 const GFX_Texture2D *const *textures) {
  GFX_Material material = {.uniform_size = layout->uniform_size};

  WGPUBindGroupEntry entries[1 + 2 * GFX_MATERIAL_MAX_TEXTURES];
  size_t count = 0;

  if (layout->uniform_size > 0) {
    WGPUBufferDescriptor buffer_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    buffer_desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    buffer_desc.size = _gfx_uniform_buffer_size(layout->uniform_size);
    material.buffer = wgpuDeviceCreateBuffer(_gfx.device, &buffer_desc);
    if (uniform_data) {
      gfx_material_set_uniform(&material, uniform_data);
    }

    entries[count] = WGPU_BIND_GROUP_ENTRY_INIT;
    entries[count].binding = 0;
    entries[count].buffer = material.buffer;
    count++;
  }

  for (uint32_t i = 0; i < layout->texture_count; i++) {
    const GFX_Texture2D *texture = textures ? textures[i] : NULL;
    if (texture == NULL || texture->view == NULL) {
      texture = &_white;
    }

    entries[count] = WGPU_BIND_GROUP_ENTRY_INIT;
    entries[count].binding = 1 + 2 * i;
    entries[count].textureView = texture->view;
    count++;

    entries[count] = WGPU_BIND_GROUP_ENTRY_INIT;
    entries[count].binding = 2 + 2 * i;
    entries[count].sampler = texture->sampler;
    count++;
  }

  WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  desc.layout = layout->layout;
  desc.entryCount = count;
  desc.entries = entries;
  material.group = wgpuDeviceCreateBindGroup(_gfx.device, &desc);

  return material;
}

void gfx_material_destroy(const GFX_Material *material) {
  wgpuBindGroupRelease(material->group);
  if (material->buffer) {
    wgpuBufferRelease(material->buffer);
  }
}

// Update the uniform, it lands before the next submitted frame
void gfx_material_set_uniform(const GFX_Material *material, const void *data) {
  wgpuQueueWriteBuffer(_gfx.queue, material->buffer, 0, data,
                       material->uniform_size);
}

// Use the material for the next draws
void gfx_material_bind(const GFX_Material *material) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 2, material->group, 0, NULL);
  }
}

// ============================================================================
// Init
// ============================================================================

void gfx_materials_init(void) {
  uint8_t white[4] = {255, 255, 255, 255};
  _white = gfx_texture2d_create(1, 1, white, GFX_TEXTURE_FILTER_NEAREST,
                                GFX_WRAP_REPEAT);
}

void gfx_materials_terminate(void) {
  gfx_texture2d_destroy(&_white);
  memset(&_white, 0, sizeof(_white));
}
