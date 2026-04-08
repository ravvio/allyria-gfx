#include "../../external/cglm/cglm.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <sys/types.h>

#define STB_IMAGE_IMPLEMENTATION

// Error checking

#define GLCall(x)                                                              \
  gfx_errors_clear();                                                          \
  x;                                                                           \
  gfx_errors_log(#x, __FILE__, __LINE__);

void gfx_errors_clear();
int gfx_errors_check();
int gfx_errors_log(const char *function, const char *file, int line);

// GL

u_int32_t gfx_gltype_get_size(u_int32_t type);

// Window

typedef void GFX_Window;

GFX_Window *gfx_window_create(int w, int h, char *window_name);
void gfx_window_size(GFX_Window *window, int *w, int *h);
void gfx_window_size_set(GFX_Window *window, int w, int h);
int gfx_window_should_close(GFX_Window *win);
void gfx_window_swap(GFX_Window *window);

void gfx_events_poll();
void gfx_terminate();

// Vertex Buffer

typedef struct {
  u_int32_t renderer_id;
} GFX_VertexBuffer;

GFX_VertexBuffer gfx_vertex_buffer_create(u_int32_t size, const void *data);
void gfx_vertex_buffer_destroy(GFX_VertexBuffer *vb);
void gfx_vertex_buffer_bind(GFX_VertexBuffer *vb);
void gfx_vertex_buffer_unbind();

// Index Buffer

typedef struct {
  u_int32_t renderer_id;
  // Number of indices inside our buffer
  u_int32_t count;
} GFX_IndexBuffer;

GFX_IndexBuffer gfx_index_buffer_create(const u_int32_t *data, u_int32_t count);
void gfx_index_buffer_destroy(GFX_IndexBuffer *vb);
void gfx_index_buffer_bind(const GFX_IndexBuffer *vb);
void gfx_index_buffer_unbind();
u_int32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *vb);

// Vertex Layout and Vertex Array

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
void gfx_vertex_layout_destroy(GFX_VertexLayout *layout);
void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, u_int32_t count);
const GFX_VertexLayoutElement *
gfx_vertex_layout_get_elements(const GFX_VertexLayout *layout);
u_int32_t gfx_vertex_layout_get_stride(GFX_VertexLayout *layout);

typedef struct {
  u_int32_t renderer_id;
} GFX_VertexArray;

GFX_VertexArray gfx_vertex_array_create();
void gfx_vertex_array_destroy(const GFX_VertexArray *vao);
void gfx_vertex_array_bind(const GFX_VertexArray *vao);
void gfx_vertex_array_unbind();
void gfx_vertex_array_add_buffer(GFX_VertexArray *vao, GFX_VertexBuffer *vb,
                                 GFX_VertexLayout *layout);

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
void gfx_shader_uniform_set_2f(const GFX_Shader *shader, const char *name,
                               float v0, float v1);
void gfx_shader_uniform_set_vec_1f(const GFX_Shader *shader, const char *name,
                                   u_int32_t count, float *v0);
void gfx_shader_uniform_set_mat4(const GFX_Shader *shader, const char *name,
                                 int transpose, mat4 value);

// Texture

typedef struct {
} GFX_Texture;

// Sprite

typedef struct {
  GFX_Shader *shader;
  GFX_Texture *texture;
} GFX_Sprite;

typedef struct {
  GFX_VertexArray *vao;
} GFX_SpriteRenderer;
