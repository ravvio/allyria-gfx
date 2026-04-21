#ifndef GFX_GRAPHICS_DECL
#define GFX_GRAPHICS_DECL

#include "../gfx_common.h"

// Color

typedef vec3 GFX_Color_RGB;
typedef vec4 GFX_Color_RGBA;

// Graphics

GFX_Error gfx_graphics_init();
void gfx_graphics_terminate();

// Features

enum GFX_Graphics_Feature {
  GFX_GRAPHICS_FEATURE_DEPTH_TESTING,
  GFX_GRAPHICS_FEATURE_CULL_BACK,
};
void gfx_graphics_enable(enum GFX_Graphics_Feature);

// Window and context

typedef void GFX_Window;

GFX_Window *gfx_window_init(int w, int h, char *window_name);

void gfx_window_size(GFX_Window *window, int *w, int *h);
void gfx_window_size_set(GFX_Window *window, int w, int h);
int gfx_window_should_close(GFX_Window *window);
void gfx_window_swap_buffers(GFX_Window *window);
void gfx_window_events_poll();

void gfx_color_buffer_clear();
void gfx_depth_buffer_clear();
void gfx_buffers_clear();
void gfx_color_buffer_set_clear(GFX_Color_RGBA color);
void gfx_depth_buffer_set_clear(float_t depth);

// Vertex Buffer (VBO)

typedef struct {
  u_int32_t renderer_id;
} GFX_VertexBuffer;

GFX_VertexBuffer gfx_vertex_buffer_create(u_int32_t size, const void *data);
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vb);
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vb);
void gfx_vertex_buffer_unbind();

// Index Buffer (IBO)

typedef struct {
  u_int32_t renderer_id;
  // Number of indices inside our buffer
  u_int32_t count;
} GFX_IndexBuffer;

GFX_IndexBuffer gfx_index_buffer_create(u_int32_t count, const u_int32_t *data);
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib);
void gfx_index_buffer_bind(const GFX_IndexBuffer *ib);
void gfx_index_buffer_unbind();
u_int32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *ib);

// Vertex Layout

typedef struct {
  u_int32_t count;
  u_int32_t type;
  u_int8_t normalized;
} GFX_VertexLayoutElement;

typedef struct {
  u_int32_t count;
  u_int32_t stride;
  GFX_VertexLayoutElement *elements;
} GFX_VertexLayout;

GFX_VertexLayout gfx_vertex_layout_create();
void gfx_vertex_layout_destroy(const GFX_VertexLayout *layout);
void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, u_int32_t count,
                                GFX_Boolean normalized);
const GFX_VertexLayoutElement *
gfx_vertex_layout_get_elements(const GFX_VertexLayout *layout);
u_int32_t gfx_vertex_layout_get_stride(const GFX_VertexLayout *layout);

// Vertex Array (VAO)

typedef struct {
  u_int32_t renderer_id;
} GFX_VertexArray;

GFX_VertexArray gfx_vertex_array_create();
void gfx_vertex_array_destroy(const GFX_VertexArray *vao);
void gfx_vertex_array_bind(const GFX_VertexArray *vao);
void gfx_vertex_array_unbind();
void gfx_vertex_array_add_buffer(const GFX_VertexArray *vao,
                                 const GFX_VertexBuffer *vb,
                                 const GFX_VertexLayout *layout);

// Shader

typedef struct {
  const char *vert_filepath;
  const char *frag_filepath;
  u_int32_t renderer_id;
  // Caching for uniforms
} GFX_Shader;

GFX_Shader gfx_shader_create(const char *vert_filepath,
                             const char *frag_filepath);
void gfx_shader_destroy(const GFX_Shader *shader);
void gfx_shader_bind(const GFX_Shader *shader);
void gfx_shader_unbind();
u_int32_t gfx_shader_uniform_get_location(const GFX_Shader *shader,
                                          const char *name);
void gfx_shader_uniform_set_int(const GFX_Shader *shader, const char *name,
                                int v);
void gfx_shader_uniform_set_float(const GFX_Shader *shader, const char *name,
                                  float v);
void gfx_shader_uniform_set_vec2(const GFX_Shader *shader, const char *name,
                                 vec2 v);
void gfx_shader_uniform_set_vec3(const GFX_Shader *shader, const char *name,
                                 vec3 v);
void gfx_shader_uniform_set_vec4(const GFX_Shader *shader, const char *name,
                                 vec4 v);
void gfx_shader_uniform_set_mat4(const GFX_Shader *shader, const char *name,
                                 int transpose, mat4 value);

// Texture

enum GFX_Wrap {
  GFX_WRAP_REPEAT,
  GFX_WRAP_MIRROR_REPEAT,
  GFX_WRAP_CLAMP_EDGE,
  GFX_WRAP_CLAMP_BORDER,
};

enum GFX_TextureFilter {
  GFX_TEXTURE_FILTER_LINEAR,
  GFX_TEXTURE_FILTER_NEAREST,
};

enum GFX_TextureType {
  GFX_TEXTURE_TYPE_DIFFUSE,
  GFX_TEXTURE_TYPE_SPECULAR,
};

typedef struct {
  int width;
  int height;
  enum GFX_TextureType type;
  u_int32_t renderer_id;
} GFX_Texture2D;

GFX_Texture2D gfx_texture2d_load(const char *filename,
                                 enum GFX_TextureType type,
                                 enum GFX_TextureFilter filter,
                                 enum GFX_Wrap wrap);
void gfx_texture2d_destroy(const GFX_Texture2D *texture);
void gfx_texture2d_bind(const GFX_Texture2D *texture);
void gfx_texture2d_unbind();

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
  GFX_Vertex3D *vertices;
  u_int32_t *indices;

  GFX_VertexArray _vao;
  GFX_VertexBuffer _vbo;
  GFX_IndexBuffer _ibo;
} GFX_Geometry3D;

GFX_Geometry3D gfx_geometry3d_create(u_int32_t vertices_count, GFX_Vertex3D *vertices,
                             u_int32_t indices_count, u_int32_t *indices);
void gfx_geometry3d_destroy(const GFX_Geometry3D *geometry);
void gfx_geometry3d_draw(const GFX_Geometry3D *geometry, const GFX_Shader *shader,
                     mat4 trasform_projection, mat4 trasform_view,
                     mat4 trasform_model);

GFX_Geometry3D gfx_geometry3d_shape_quad_create(float_t width, float_t height);
GFX_Geometry3D gfx_geometry3d_shape_cuboid_create(float_t width, float_t height,
                                          float_t depth);

// Geometry2D

typedef struct {
  vec2 position;
  vec2 uv;
} GFX_Vertex2D;

GFX_Vertex2D gfx_vertex2d_create(float position_x, float position_y, float uv_x,
                                 float uv_y);

typedef struct {
  GFX_Vertex2D *vertices;
  u_int32_t *indices;

  GFX_VertexArray _vao;
  GFX_VertexBuffer _vbo;
  GFX_IndexBuffer _ibo;
} GFX_Geometry2D;

GFX_Geometry2D gfx_geometry2d_create(u_int32_t vertices_count, GFX_Vertex2D *vertices,
                             u_int32_t indices_count, u_int32_t *indices);
void gfx_geometry2d_destroy(const GFX_Geometry2D *geometry);
void gfx_geometry2d_draw(const GFX_Geometry2D *geometry, const GFX_Shader *shader,
                     mat4 model, mat4 view,
                     mat4 projection);

GFX_Geometry2D gfx_geometry2d_shape_quad_create(float_t width, float_t height);

#endif
