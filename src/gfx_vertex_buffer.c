#include "../include/gfx/gfx_gl.h"

// Create and bind a vertex buffer
GFX_VertexBuffer gfx_vertex_buffer_create(u_int32_t size, const void *data) {
  // Generate buffer
  u_int32_t buffer_id;
  GLCall(glGenBuffers(1, &buffer_id));
  // Bind and set data
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer_id));
  GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));

  GFX_VertexBuffer vb = {.renderer_id = buffer_id};
  return vb;
}

// Destroy a vertex buffer
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vb) {
  GLCall(glDeleteBuffers(1, &vb->renderer_id));
}

// Bind a vertex buffer
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vb) {
  GLCall(glBindBuffer(GL_ARRAY_BUFFER, vb->renderer_id));
}

// Unbind the vertex buffer
void gfx_vertex_buffer_unbind() { GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0)); }
