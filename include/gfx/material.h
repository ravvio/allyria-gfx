#ifndef GFX_MATERIAL_DECL
#define GFX_MATERIAL_DECL

#include <gfx/core.h>
#include <gfx/texture.h>
#include <webgpu/webgpu.h>

// Material layout, the kind of material a pipeline draws with. It owns
// @group(2) of the shader:
//   @binding(0)    uniform of uniform_size bytes (only if uniform_size > 0)
//   @binding(1+2i) texture i, texture_2d<f32>
//   @binding(2+2i) sampler of texture i
// e.g. a layout (sizeof(vec4), 1) matches
//   @group(2) @binding(0) var<uniform> albedo: vec4f;
//   @group(2) @binding(1) var albedo_tex: texture_2d<f32>;
//   @group(2) @binding(2) var albedo_sampler: sampler;

#define GFX_MATERIAL_MAX_TEXTURES 4

typedef struct {
  uint32_t uniform_size;
  uint32_t texture_count;
  WGPUBindGroupLayout layout;
  // Frame, object and this material groups
  WGPUPipelineLayout pipeline_layout;
} GFX_MaterialLayout;

GFX_MaterialLayout gfx_material_layout_create(uint32_t uniform_size,
                                              uint32_t texture_count);
void gfx_material_layout_destroy(const GFX_MaterialLayout *layout);

// Material, the values for a material layout

typedef struct {
  uint32_t uniform_size;
  WGPUBuffer buffer;
  WGPUBindGroup group;
} GFX_Material;

// uniform_data has uniform_size bytes (can be NULL to zero it), textures has
// texture_count entries, NULL entries or a NULL array use a white texture
GFX_Material gfx_material_create(const GFX_MaterialLayout *layout,
                                 const void *uniform_data,
                                 const GFX_Texture2D *const *textures);
void gfx_material_destroy(const GFX_Material *material);
// Update the uniform, the last value of the frame wins
void gfx_material_set_uniform(const GFX_Material *material, const void *data);
// Use the material for the next draws of the frame
void gfx_material_bind(const GFX_Material *material);

#endif
