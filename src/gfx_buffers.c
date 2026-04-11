// Methods to create and manipulate OpenGL buffers and objects

#include "../include/gfx/gfx_gl.h"

// ============================================================================
// VBO (Vertex Buffer)
// ============================================================================

// Create and bind a vertex buffer
GFX_VertexBuffer gfx_vertex_buffer_create(u_int32_t size, const void *data) {
  // Generate buffer
  u_int32_t buffer_id;
  GLCall(glGenBuffers(1, &buffer_id));
  // Bind and set data
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer_id));
  GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));

  GFX_VertexBuffer vbo = {.renderer_id = buffer_id};
  return vbo;
}

// Destroy a vertex buffer
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vbo) {
  GLCall(glDeleteBuffers(1, &vbo->renderer_id));
}

// Bind a vertex buffer
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vbo) {
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo->renderer_id));
}

// Unbind the vertex buffer
void gfx_vertex_buffer_unbind() { GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); }

// ============================================================================
// IBO (Index Buffer)
// ============================================================================

// Create and bind an index buffer
GFX_IndexBuffer gfx_index_buffer_create(u_int32_t count,
                                        const u_int32_t *data) {
  // Generate buffer
  u_int32_t buffer_id;
  glGenBuffers(1, &buffer_id);
  // Bind and set data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u_int32_t), data,
               GL_STATIC_DRAW);

  GFX_IndexBuffer ib = {
      .renderer_id = buffer_id,
      .count = count,
  };
  return ib;
}

// Destroy a index buffer
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib) {
  glDeleteBuffers(1, &ib->renderer_id);
}

// Bind a index buffer
void gfx_index_buffer_bind(const GFX_IndexBuffer *ib) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->renderer_id);
}

// Unbind a index buffer
void gfx_index_buffer_unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

// Get count of indices inside buffer
u_int32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *ib) {
  return ib->count;
}

// ============================================================================
// VAO (Vertex Array)
// ============================================================================

// Create a vertex layout
GFX_VertexLayout gfx_vertex_layout_create() {
  GFX_VertexLayoutElement *elements = {};
  GFX_VertexLayout layout = {
      .count = 0,
      .stride = 0,
      .elements = elements,
  };
  return layout;
}

// Destroy a vertex layout, freeing its elements
void gfx_vertex_layout_destroy(const GFX_VertexLayout *layout) {
  free(layout->elements);
}

// Add a structure of `count` f32 to the layout
void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, u_int32_t count, GFX_Boolean normalized) {
  // Increase size
  layout->count += 1;
  layout->elements = realloc(layout->elements,
                             layout->count * sizeof(GFX_VertexLayoutElement));
  // Assign new element
  GFX_VertexLayoutElement el = {
      .count = count,
      .type = GL_FLOAT,
      .normalized = normalized,
  };
  layout->elements[layout->count - 1] = el;
  // Increase stride
  layout->stride += count * gfx_gl_type_get_size(GL_FLOAT);
}

// Get the elements pushed to the vertex layout
const GFX_VertexLayoutElement *
gfx_vertex_layout_get_elements(const GFX_VertexLayout *layout) {
  return layout->elements;
}

// Get the stride of the vertex layout
u_int32_t gfx_vertex_layout_get_stride(const GFX_VertexLayout *layout) {
  return layout->stride;
}

// Create and bind a vertex array (VAO)
GFX_VertexArray gfx_vertex_array_create() {
  u_int32_t id;
  GLCall(glGenVertexArrays(1, &id));
  GLCall(glBindVertexArray(id));

  GFX_VertexArray va = {.renderer_id = id};
  return va;
}

// Destroy a vertex array (VAO)
void gfx_vertex_array_destroy(const GFX_VertexArray *vao) {
  GLCall(glDeleteVertexArrays(1, &vao->renderer_id));
}

// Bind a vertex array (VAO)
void gfx_vertex_array_bind(const GFX_VertexArray *vao) {
  GLCall(glBindVertexArray(vao->renderer_id));
}

// Unbind the vertex array (VAO)
void gfx_vertex_array_unbind() { GLCall(glBindVertexArray(0)); }

// Add a vertex buffer (VBO) to the vertex array (VAO) using a given layout
void gfx_vertex_array_add_buffer(const GFX_VertexArray *vao, const GFX_VertexBuffer *vbo,
                                 const GFX_VertexLayout *layout) {
  gfx_vertex_array_bind(vao);
  gfx_vertex_buffer_bind(vbo);

  u_int64_t offset = 0;
  for (u_int32_t i = 0; i < layout->count; i++) {
    GFX_VertexLayoutElement el = layout->elements[i];
    GLCall(glVertexAttribPointer(i, el.count, el.type, el.normalized,
                                 layout->stride, (const void *)offset));
    GLCall(glEnableVertexAttribArray(i));
    offset += el.count * gfx_gl_type_get_size(el.type);
  }
}
