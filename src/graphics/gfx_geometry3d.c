#include "../../include/gfx/graphics/gfx_wgpu.h"

const GFX_VertexLayout *gfx_geometry3d_layout(void) {
  static GFX_VertexLayout layout = {0};
  if (layout.count == 0) {
    // Position
    gfx_vertex_layout_push_f32(&layout, 3);
    // Normal
    gfx_vertex_layout_push_f32(&layout, 3);
    // UV
    gfx_vertex_layout_push_f32(&layout, 2);
  }
  return &layout;
}

GFX_Vertex3D gfx_vertex3d_create(float position_x, float position_y,
                                 float position_z, float normal_x,
                                 float normal_y, float normal_z, float uv_x,
                                 float uv_y) {
  GFX_Vertex3D vertex = {
      .position = GLM_VEC3_ZERO_INIT,
      .normal = GLM_VEC3_ZERO_INIT,
      .uv = GLM_VEC2_ZERO_INIT,
  };

  vec3 position = {position_x, position_y, position_z};
  glm_vec3_copy(position, vertex.position);
  vec3 normal = {normal_x, normal_y, normal_z};
  glm_vec3_copy(normal, vertex.normal);
  vec2 uv = {uv_x, uv_y};
  glm_vec2_copy(uv, vertex.uv);

  return vertex;
}

// Upload the vertices and indices, the arrays can be freed afterwards
GFX_Geometry3D gfx_geometry3d_create(u_int32_t vertices_count, GFX_Vertex3D *vertices,
                             u_int32_t indices_count, u_int32_t *indices) {
  GFX_Geometry3D geometry = {
      ._vbo = gfx_vertex_buffer_create(vertices_count * sizeof(GFX_Vertex3D),
                                       vertices),
      ._ibo = gfx_index_buffer_create(indices_count, indices),
  };
  return geometry;
}

void gfx_geometry3d_destroy(const GFX_Geometry3D *geometry) {
  gfx_vertex_buffer_destroy(&geometry->_vbo);
  gfx_index_buffer_destroy(&geometry->_ibo);
}

// Draw with the bound pipeline, between gfx_frame_begin and gfx_frame_end
void gfx_geometry3d_draw(const GFX_Geometry3D *geometry) {
  gfx_vertex_buffer_bind(&geometry->_vbo);
  gfx_index_buffer_bind(&geometry->_ibo);
  gfx_draw_indexed(geometry->_ibo.count);
}

GFX_Geometry3D gfx_geometry3d_shape_quad_create(float_t width, float_t height) {
  GFX_Vertex3D vertices[4] = {
      // front
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, +1.0, 0.0,
                          1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, +1.0, 0.0,
                          0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, +1.0, 1.0,
                          0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, +1.0, 1.0,
                          1.0),
  };
  u_int32_t indices[6] = {0, 1, 2, 2, 3, 0};

  return gfx_geometry3d_create(4, vertices, 6, indices);
}

GFX_Geometry3D gfx_geometry3d_shape_cuboid_create(float_t width, float_t height,
                                          float_t depth) {
  GFX_Vertex3D vertices[24] = {
      // front
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, +depth / 2.0, 0.0, 0.0,
                          +1.0, 0.0, 1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, +depth / 2.0, 0.0, 0.0,
                          +1.0, 0.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, +depth / 2.0, 0.0, 0.0,
                          +1.0, 1.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, +depth / 2.0, 0.0, 0.0,
                          +1.0, 1.0, 1.0),
      // right
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, +depth / 2.0, +1.0, 0.0,
                          0.0, 0.0, 1.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, +depth / 2.0, +1.0, 0.0,
                          0.0, 0.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, -depth / 2.0, +1.0, 0.0,
                          0.0, 1.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, -depth / 2.0, +1.0, 0.0,
                          0.0, 1.0, 1.0),
      // back
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, -depth / 2.0, 0.0, 0.0,
                          -1.0, 0.0, 1.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, -depth / 2.0, 0.0, 0.0,
                          -1.0, 0.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, -depth / 2.0, 0.0, 0.0,
                          -1.0, 1.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, -depth / 2.0, 0.0, 0.0,
                          -1.0, 1.0, 1.0),
      // left
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, -depth / 2.0, -1.0, 0.0,
                          0.0, 0.0, 1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, -depth / 2.0, -1.0, 0.0,
                          0.0, 0.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, +depth / 2.0, -1.0, 0.0,
                          0.0, 1.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, +depth / 2.0, -1.0, 0.0,
                          0.0, 1.0, 1.0),
      // top
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, -depth / 2.0, 0.0, +1.0,
                          0.0, 0.0, 1.0),
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, +depth / 2.0, 0.0, +1.0,
                          0.0, 0.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, +depth / 2.0, 0.0, +1.0,
                          0.0, 1.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, -depth / 2.0, 0.0, +1.0,
                          0.0, 1.0, 1.0),
      // bottom
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, +depth / 2.0, 0.0, -1.0,
                          0.0, 0.0, 1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, -depth / 2.0, 0.0, -1.0,
                          0.0, 0.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, -depth / 2.0, 0.0, -1.0,
                          0.0, 1.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, +depth / 2.0, 0.0, -1.0,
                          0.0, 1.0, 1.0),
  };

  u_int32_t indices[36] = {// front
                           0, 1, 2, 2, 3, 0,
                           // right
                           4, 5, 6, 6, 7, 4,
                           // back
                           8, 9, 10, 10, 11, 8,
                           // left
                           12, 13, 14, 14, 15, 12,
                           // top
                           16, 17, 18, 18, 19, 16,
                           // bottom
                           20, 21, 22, 22, 23, 20};

  return gfx_geometry3d_create(24, vertices, 36, indices);
}
