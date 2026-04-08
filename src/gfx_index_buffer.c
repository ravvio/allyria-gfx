#include <assert.h>
#include <sys/types.h>

#include "../include/gfx/gfx.h"

// Create and bind an index buffer
GFX_IndexBuffer gfx_index_buffer_create(const u_int32_t *data,
                                        u_int32_t count) {
  assert(sizeof(u_int32_t) == sizeof(GLuint));

  // Generate buffer
  u_int32_t buffer_id;
  GLCall(glGenBuffers(1, &buffer_id));
  // Bind and set data
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id));
  GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u_int32_t), data,
                      GL_STATIC_DRAW));

  GFX_IndexBuffer ib = {
      .renderer_id = buffer_id,
      .count = count,
  };
  return ib;
}

// Destroy a index buffer
void gfx_index_buffer_destroy(GFX_IndexBuffer *vb) {
  GLCall(glDeleteBuffers(1, &vb->renderer_id));
}

// Bind a index buffer
void gfx_index_buffer_bind(const GFX_IndexBuffer *vb) {
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb->renderer_id));
}

// Unbind a index buffer
void gfx_index_buffer_unbind() {
  GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

// Get count of indices inside buffer
u_int32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *vb) {
  return vb->count;
}
