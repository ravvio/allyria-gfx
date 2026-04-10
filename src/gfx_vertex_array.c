#include "../include/gfx/gfx_gl.h"

GFX_VertexLayout gfx_vertex_layout_create() {
  GFX_VertexLayoutElement *elements = {};
  GFX_VertexLayout layout = {
      .count = 0,
      .stride = 0,
      .elements = elements,
  };
  return layout;
}

void gfx_vertex_layout_destroy(GFX_VertexLayout *layout) {
  free(layout->elements);
}

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

const GFX_VertexLayoutElement *
gfx_vertex_layout_get_elements(const GFX_VertexLayout *layout) {
  return layout->elements;
}

u_int32_t gfx_vertex_layout_get_stride(GFX_VertexLayout *layout) {
  return layout->stride;
}

GFX_VertexArray gfx_vertex_array_create() {
  u_int32_t id;
  GLCall(glGenVertexArrays(1, &id));
  GLCall(glBindVertexArray(id));

  GFX_VertexArray va = {.renderer_id = id};
  return va;
}

void gfx_vertex_array_destroy(const GFX_VertexArray *vao) {
  GLCall(glDeleteVertexArrays(1, &vao->renderer_id));
}

void gfx_vertex_array_bind(const GFX_VertexArray *vao) {
  GLCall(glBindVertexArray(vao->renderer_id));
}

void gfx_vertex_array_unbind() { GLCall(glBindVertexArray(0)); }

void gfx_vertex_array_add_buffer(GFX_VertexArray *vao, GFX_VertexBuffer *vb,
                                 GFX_VertexLayout *layout) {
  gfx_vertex_array_bind(vao);
  gfx_vertex_buffer_bind(vb);

  u_int64_t offset = 0;
  for (u_int32_t i = 0; i < layout->count; i++) {
    GFX_VertexLayoutElement el = layout->elements[i];
    GLCall(glVertexAttribPointer(i, el.count, el.type, el.normalized,
                                 layout->stride, (const void *)offset));
    GLCall(glEnableVertexAttribArray(i));
    offset += el.count * gfx_gl_type_get_size(el.type);
  }
}
