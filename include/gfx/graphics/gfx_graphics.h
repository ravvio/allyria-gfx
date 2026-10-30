#ifndef GFX_GRAPHICS_DECL
#define GFX_GRAPHICS_DECL

#include "../gfx_common.h"
#include <webgpu/webgpu.h>

// Color

typedef vec3 GFX_Color_RGB;
typedef vec4 GFX_Color_RGBA;

// Graphics

GFX_Error gfx_graphics_init(void);
void gfx_graphics_terminate(void);

// Features

enum GFX_Graphics_Feature {
  GFX_GRAPHICS_FEATURE_DEPTH_TESTING,
  GFX_GRAPHICS_FEATURE_CULL_BACK,
  GFX_GRAPHICS_FEATURE_ALPHA_BLEND,
};
void gfx_graphics_enable(enum GFX_Graphics_Feature);

// Window and context

typedef void GFX_Window;

GFX_Window *gfx_window_init(int w, int h, const char *window_name);
void gfx_window_destroy(GFX_Window* window);

void gfx_window_size(GFX_Window *window, int *w, int *h);
void gfx_window_size_set(GFX_Window *window, int w, int h);
void gfx_window_scale(GFX_Window *window, float *sx, float *sy);
int gfx_window_should_close(GFX_Window *window);
void gfx_window_events_poll(void);

// Frame

void gfx_color_buffer_set_clear(const GFX_Color_RGBA color);
void gfx_frame_begin(GFX_Window *window);
void gfx_frame_end(GFX_Window *window);

// Vertex Buffer

typedef struct {
  WGPUBuffer buffer;
} GFX_VertexBuffer;

// data can be NULL to only allocate
GFX_VertexBuffer gfx_vertex_buffer_create(u_int32_t size, const void *data);
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vb);
// Overwrite the start of the buffer, like uniforms the last write of the
// frame wins
void gfx_vertex_buffer_write(const GFX_VertexBuffer *vb, const void *data,
                             u_int32_t size);
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vb);

// Index Buffer (u32 indices)

typedef struct {
  WGPUBuffer buffer;
  // Number of indices inside our buffer
  u_int32_t count;
} GFX_IndexBuffer;

GFX_IndexBuffer gfx_index_buffer_create(u_int32_t count, const u_int32_t *data);
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib);
void gfx_index_buffer_bind(const GFX_IndexBuffer *ib);
u_int32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *ib);

// Vertex Layout, zero initialize then push attributes (shader locations 0..n)

#define GFX_VERTEX_LAYOUT_MAX_ATTRIBUTES 8

typedef struct {
  // Zero advances per vertex, WGPUVertexStepMode_Instance per instance
  WGPUVertexStepMode step;
  u_int32_t count;
  u_int32_t stride;
  WGPUVertexAttribute attributes[GFX_VERTEX_LAYOUT_MAX_ATTRIBUTES];
} GFX_VertexLayout;

void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, u_int32_t count);

// Shader, a WGSL file with entry points vs_main and fs_main

typedef struct {
  const char *filepath;
  WGPUShaderModule module;
} GFX_Shader;

GFX_Shader gfx_shader_create(const char *filepath);
void gfx_shader_destroy(const GFX_Shader *shader);

// Pipeline, shader + vertex layout + material layout (NULL for none) +
// enabled graphics features. Create it after the window, it renders to the
// window surface format.

typedef struct GFX_MaterialLayout GFX_MaterialLayout;

typedef struct {
  WGPURenderPipeline pipeline;
} GFX_Pipeline;

GFX_Pipeline gfx_pipeline_create(const GFX_Shader *shader,
                                 const GFX_VertexLayout *layout,
                                 const GFX_MaterialLayout *material);
void gfx_pipeline_destroy(const GFX_Pipeline *pipeline);
void gfx_pipeline_bind(const GFX_Pipeline *pipeline);

// Uniforms, every shader can declare:
//   @group(0) @binding(0) var<uniform> view_projection: mat4x4f;
//   @group(1) @binding(0) var<uniform> model: mat4x4f;

// Camera projection * view, once per frame (last call wins)
void gfx_frame_set_view_projection(mat4 view_projection);
// Before each draw that needs its own model, between frame begin and end
void gfx_draw_set_model(mat4 model);

// Draw, between gfx_frame_begin and gfx_frame_end

void gfx_draw_indexed(u_int32_t count);
// Without index buffer, instances read per-instance vertex buffers from
// first_instance on
void gfx_draw_instanced(u_int32_t vertex_count, u_int32_t instance_count,
                        u_int32_t first_instance);

// Geometry2D

typedef struct {
  vec2 position;
  vec2 uv;
} GFX_Vertex2D;

GFX_Vertex2D gfx_vertex2d_create(float position_x, float position_y, float uv_x,
                                 float uv_y);

typedef struct {
  GFX_VertexBuffer _vbo;
  GFX_IndexBuffer _ibo;
} GFX_Geometry2D;

// Vertex layout of GFX_Vertex2D (locations 0 position, 1 uv) for pipelines
const GFX_VertexLayout *gfx_geometry2d_layout(void);

GFX_Geometry2D gfx_geometry2d_create(u_int32_t vertices_count, GFX_Vertex2D *vertices,
                             u_int32_t indices_count, u_int32_t *indices);
void gfx_geometry2d_destroy(const GFX_Geometry2D *geometry);
void gfx_geometry2d_draw(const GFX_Geometry2D *geometry);

GFX_Geometry2D gfx_geometry2d_shape_quad_create(float_t width, float_t height);

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
GFX_Texture2D gfx_texture2d_create(int width, int height,
                                   const u_int8_t *pixels,
                                   enum GFX_TextureFilter filter,
                                   enum GFX_Wrap wrap);
// On failure the texture is zeroed, materials then use white
GFX_Texture2D gfx_texture2d_load(const char *filename,
                                 enum GFX_TextureFilter filter,
                                 enum GFX_Wrap wrap);
void gfx_texture2d_destroy(const GFX_Texture2D *texture);

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

// typedef'd as GFX_MaterialLayout in the pipeline section
struct GFX_MaterialLayout {
  u_int32_t uniform_size;
  u_int32_t texture_count;
  WGPUBindGroupLayout layout;
  // Frame, object and this material groups
  WGPUPipelineLayout pipeline_layout;
};

GFX_MaterialLayout gfx_material_layout_create(u_int32_t uniform_size,
                                              u_int32_t texture_count);
void gfx_material_layout_destroy(const GFX_MaterialLayout *layout);

// Material, the values for a material layout

typedef struct {
  u_int32_t uniform_size;
  WGPUBuffer buffer;
  WGPUBindGroup group;
} GFX_Material;

// uniform_data has uniform_size bytes (can be NULL to zero it), textures has
// texture_count entries, NULL entries or a NULL array use a white texture
GFX_Material gfx_material_create(const GFX_MaterialLayout *layout,
                                 const void *uniform_data,
                                 const GFX_Texture2D *const *textures);
void gfx_material_destroy(const GFX_Material *material);
// Update the uniform, like the camera the last value of the frame wins
void gfx_material_set_uniform(const GFX_Material *material, const void *data);
// Use the material for the next draws, between frame begin and end
void gfx_material_bind(const GFX_Material *material);

// Geometry3D

typedef struct {
  vec3 position;
  vec3 normal;
  vec2 uv;
} GFX_Vertex3D;

GFX_Vertex3D gfx_vertex3d_create(float position_x, float position_y,
                                 float position_z, float normal_x,
                                 float normal_y, float normal_z, float uv_x,
                                 float uv_y);

typedef struct {
  GFX_VertexBuffer _vbo;
  GFX_IndexBuffer _ibo;
} GFX_Geometry3D;

// Vertex layout of GFX_Vertex3D (locations 0 position, 1 normal, 2 uv)
const GFX_VertexLayout *gfx_geometry3d_layout(void);

GFX_Geometry3D gfx_geometry3d_create(u_int32_t vertices_count, GFX_Vertex3D *vertices,
                             u_int32_t indices_count, u_int32_t *indices);
void gfx_geometry3d_destroy(const GFX_Geometry3D *geometry);
void gfx_geometry3d_draw(const GFX_Geometry3D *geometry);

GFX_Geometry3D gfx_geometry3d_shape_quad_create(float_t width, float_t height);
GFX_Geometry3D gfx_geometry3d_shape_cuboid_create(float_t width, float_t height,
                                          float_t depth);

#endif
