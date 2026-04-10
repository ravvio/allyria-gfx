#include "../include/gfx/gfx_gl.h"

static GFX_Boolean _gfx_vertex3d_layout_init = GFX_FALSE;
static GFX_VertexLayout _gfx_vertex3d_layout;

GFX_VertexLayout *gfx_vertex3d_layout() {
  if (_gfx_vertex3d_layout_init == GFX_TRUE) {
    return &_gfx_vertex3d_layout;
  }

  _gfx_vertex3d_layout = gfx_vertex_layout_create();
  // Position
  gfx_vertex_layout_push_f32(&_gfx_vertex3d_layout, 3, false);
  // Normal
  gfx_vertex_layout_push_f32(&_gfx_vertex3d_layout, 3, false);
  // UV
  gfx_vertex_layout_push_f32(&_gfx_vertex3d_layout, 2, false);

  fprintf(stderr, "[i] vertex3D layout initialized\n");
  return &_gfx_vertex3d_layout;
}

GFX_Vertex3D gfx_vertex3d_create(float position_x, float position_y,
                                 float position_z, float normal_x,
                                 float normal_y, float normal_z,
                                 float uv_x, float uv_y) {
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

GFX_Mesh3D gfx_mesh3d_create(u_int32_t vertices_count, GFX_Vertex3D *vertices,
                             u_int32_t indices_count, u_int32_t *indices) {
  GFX_VertexArray vao = gfx_vertex_array_create();
  GFX_VertexBuffer vbo =
      gfx_vertex_buffer_create(vertices_count * sizeof(GFX_Vertex3D), vertices);
  GFX_IndexBuffer ibo = gfx_index_buffer_create(indices_count, indices);

  gfx_vertex_array_add_buffer(&vao, &vbo, gfx_vertex3d_layout());

  GFX_Mesh3D mesh = {
      .vertices = vertices,
      .vertices_count = vertices_count,
      .indices = indices,
      .indices_count = indices_count,
      ._vao = vao,
      ._vbo = vbo,
      ._ibo = ibo,
  };
  return mesh;
}

void gfx_mesh3d_destroy(GFX_Mesh3D *mesh) {
  gfx_vertex_array_destroy(&mesh->_vao);
  gfx_vertex_buffer_destroy(&mesh->_vbo);
  gfx_index_buffer_destroy(&mesh->_ibo);
}

GFX_Mesh3D gfx_mesh3d_shape_quad_create(float_t width, float_t height) {
  GFX_Vertex3D vertices[4] = {
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, -1.0,
                          0.0, 1.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, -1.0,
                          1.0, 1.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, -1.0,
                          1.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, -1.0,
                          0.0, 0.0),
  };
  u_int32_t indices[6] = {0, 1, 2, 2, 3, 0};

  return gfx_mesh3d_create(4, vertices, 6, indices);
}

void gfx_mesh3d_draw(GFX_Mesh3D *mesh, GFX_Shader *shader) {
  gfx_shader_bind(shader);
  gfx_vertex_array_bind(&mesh->_vao);
  gfx_index_buffer_bind(&mesh->_ibo);
  glDrawElements(GL_TRIANGLES, mesh->indices_count, GL_UNSIGNED_INT, NULL);
}
