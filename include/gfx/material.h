#ifndef GFX_MATERIAL_DECL
#define GFX_MATERIAL_DECL

#include <gfx/core.h>
#include <gfx/render.h>
#include <gfx/texture.h>
#include <webgpu/webgpu.h>

// Material, a shader and the values of its material bindings. The shader
// must outlive the material.
typedef struct {
  const GFX_Shader *shader;
  WGPUBuffer buffer;
  WGPUBindGroup group;
} GFX_Material;

// uniform_data has the shader's uniform_size bytes (can be NULL to zero it),
// textures has its texture_count entries, NULL entries or a NULL array use a
// white texture
GFX_Material gfx_material_create(const GFX_Shader *shader,
                                 const void *uniform_data,
                                 const GFX_Texture2D *const *textures);
void gfx_material_destroy(const GFX_Material *material);
// Update the uniform, the last value of the frame wins
void gfx_material_set_uniform(const GFX_Material *material, const void *data);
// Use the material for the next draws of the frame
void gfx_material_bind(const GFX_Material *material);

// Built-in unlit shader for GFX_Vertex3D meshes
typedef struct {
  vec4 albedo;
} GFX_UnlitUniform;

const GFX_Shader *gfx_shader_unlit(void);

#endif
