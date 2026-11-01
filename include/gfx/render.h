#ifndef GFX_RENDER_DECL
#define GFX_RENDER_DECL

#include <gfx/core.h>
#include <gfx/material.h>
#include <webgpu/webgpu.h>

// Vertex Buffer

typedef struct {
  WGPUBuffer buffer;
} GFX_VertexBuffer;

// data can be NULL to only allocate
GFX_VertexBuffer gfx_vertex_buffer_create(uint32_t size, const void *data);
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vb);
// Overwrite the start of the buffer, like uniforms the last write of the
// frame wins
void gfx_vertex_buffer_write(const GFX_VertexBuffer *vb, const void *data,
                             uint32_t size);
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vb);

// Index Buffer (u32 indices)

typedef struct {
  WGPUBuffer buffer;
  // Number of indices inside our buffer
  uint32_t count;
} GFX_IndexBuffer;

GFX_IndexBuffer gfx_index_buffer_create(uint32_t count, const uint32_t *data);
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib);
void gfx_index_buffer_bind(const GFX_IndexBuffer *ib);
uint32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *ib);

// Vertex Layout, zero initialize then push attributes (shader locations 0..n)

#define GFX_VERTEX_LAYOUT_MAX_ATTRIBUTES 8

typedef struct {
  // Zero advances per vertex, WGPUVertexStepMode_Instance per instance
  WGPUVertexStepMode step;
  uint32_t count;
  uint32_t stride;
  WGPUVertexAttribute attributes[GFX_VERTEX_LAYOUT_MAX_ATTRIBUTES];
} GFX_VertexLayout;

void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, uint32_t count);

// Shader, WGSL with entry points vs_main and fs_main

typedef struct {
  // Shown in error messages, the file path for loaded shaders
  const char *label;
  WGPUShaderModule module;
} GFX_Shader;

// Load a WGSL file
GFX_Shader gfx_shader_create(const char *filepath);
// WGSL source in memory (null terminated), label must outlive the shader
GFX_Shader gfx_shader_create_wgsl(const char *label, const char *source);
void gfx_shader_destroy(const GFX_Shader *shader);

// Pipeline, shader + vertex layout + material layout (NULL for none) +
// enabled graphics features. Create it after the window, it renders to the
// window surface format.

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

void gfx_draw_indexed(uint32_t count);
// Without index buffer, instances read per-instance vertex buffers from
// first_instance on
void gfx_draw_instanced(uint32_t vertex_count, uint32_t instance_count,
                        uint32_t first_instance);

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

GFX_Geometry2D gfx_geometry2d_create(uint32_t vertices_count,
                                     GFX_Vertex2D *vertices,
                                     uint32_t indices_count, uint32_t *indices);
void gfx_geometry2d_destroy(const GFX_Geometry2D *geometry);
void gfx_geometry2d_draw(const GFX_Geometry2D *geometry);

GFX_Geometry2D gfx_geometry2d_shape_quad_create(float width, float height);

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

GFX_Geometry3D gfx_geometry3d_create(uint32_t vertices_count,
                                     GFX_Vertex3D *vertices,
                                     uint32_t indices_count, uint32_t *indices);
void gfx_geometry3d_destroy(const GFX_Geometry3D *geometry);
void gfx_geometry3d_draw(const GFX_Geometry3D *geometry);

GFX_Geometry3D gfx_geometry3d_shape_quad_create(float width, float height);
GFX_Geometry3D gfx_geometry3d_shape_cuboid_create(float width, float height,
                                                  float depth);

#endif
