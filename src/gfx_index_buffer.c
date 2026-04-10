#include "../include/gfx/gfx_gl.h"

// Create and bind an index buffer
GFX_IndexBuffer gfx_index_buffer_create(u_int32_t count,
                                        const u_int32_t *data) {
  assert(sizeof(u_int32_t) == sizeof(GLuint));

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
void gfx_index_buffer_destroy(GFX_IndexBuffer *ib) {
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
