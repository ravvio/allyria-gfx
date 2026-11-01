#include <gfx/render.h>

const GFX_VertexLayout *gfx_geometry2d_layout(void) {
  static GFX_VertexLayout layout = {0};
  if (layout.count == 0) {
    // Position
    gfx_vertex_layout_push_f32(&layout, 2);
    // UV
    gfx_vertex_layout_push_f32(&layout, 2);
  }
  return &layout;
}

GFX_Vertex2D gfx_vertex2d_create(float position_x, float position_y, float uv_x,
                                 float uv_y) {
  GFX_Vertex2D vertex = {
      .position = GLM_VEC2_ZERO_INIT,
      .uv = GLM_VEC2_ZERO_INIT,
  };

  vec2 position = {position_x, position_y};
  glm_vec2_copy(position, vertex.position);
  vec2 uv = {uv_x, uv_y};
  glm_vec2_copy(uv, vertex.uv);

  return vertex;
}

// Upload the vertices and indices, the arrays can be freed afterwards
GFX_Geometry2D gfx_geometry2d_create(uint32_t vertices_count,
                                     GFX_Vertex2D *vertices,
                                     uint32_t indices_count,
                                     uint32_t *indices) {
  GFX_Geometry2D geometry = {
      ._vbo = gfx_vertex_buffer_create(vertices_count * sizeof(GFX_Vertex2D),
                                       vertices),
      ._ibo = gfx_index_buffer_create(indices_count, indices),
  };
  return geometry;
}

void gfx_geometry2d_destroy(const GFX_Geometry2D *geometry) {
  gfx_vertex_buffer_destroy(&geometry->_vbo);
  gfx_index_buffer_destroy(&geometry->_ibo);
}

// Draw with the bound pipeline, between gfx_frame_begin and gfx_frame_end
void gfx_geometry2d_draw(const GFX_Geometry2D *geometry) {
  gfx_vertex_buffer_bind(&geometry->_vbo);
  gfx_index_buffer_bind(&geometry->_ibo);
  gfx_draw_indexed(geometry->_ibo.count);
}

GFX_Geometry2D gfx_geometry2d_shape_quad_create(float_t width, float_t height) {
  GFX_Vertex2D vertices[4] = {
      gfx_vertex2d_create(-width / 2.0, -height / 2.0, 0.0, 0.0),
      gfx_vertex2d_create(-width / 2.0, +height / 2.0, 0.0, 1.0),
      gfx_vertex2d_create(+width / 2.0, -height / 2.0, 1.0, 0.0),
      gfx_vertex2d_create(+width / 2.0, +height / 2.0, 1.0, 1.0),
  };
  uint32_t indices[6] = {0, 1, 2, 2, 3, 1};

  return gfx_geometry2d_create(4, vertices, 6, indices);
}
