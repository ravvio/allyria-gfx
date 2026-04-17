#include "../include/gfx/gfx_gl.h"

static GFX_Boolean __gfx_vertex2d_layout_init = GFX_FALSE;
static GFX_VertexLayout __gfx_vertex2d_layout;

GFX_VertexLayout *_gfx_vertex2d_layout() {
  if (__gfx_vertex2d_layout_init == GFX_TRUE) {
    return &__gfx_vertex2d_layout;
  }

  __gfx_vertex2d_layout = gfx_vertex_layout_create();
  // Position
  gfx_vertex_layout_push_f32(&__gfx_vertex2d_layout, 2, false);
  // UV
  gfx_vertex_layout_push_f32(&__gfx_vertex2d_layout, 2, false);

  fprintf(stderr, "[i] vertex2D layout initialized\n");
  return &__gfx_vertex2d_layout;
}

GFX_Vertex2D gfx_vertex2d_create(float position_x, float position_y, float uv_x, float uv_y) {
  GFX_Vertex2D vertex = {
      .position = GLM_VEC2_ZERO_INIT,
      .uv = GLM_VEC2_ZERO_INIT,
  };

  vec2 position = {position_x, position_y };
  glm_vec2_copy(position, vertex.position);
  vec2 uv = {uv_x, uv_y};
  glm_vec2_copy(uv, vertex.uv);

  return vertex;
}

GFX_Mesh2D gfx_mesh2d_create(u_int32_t vertices_count, GFX_Vertex2D *vertices,
                             u_int32_t indices_count, u_int32_t *indices) {
  GFX_VertexArray vao = gfx_vertex_array_create();
  GFX_VertexBuffer vbo =
      gfx_vertex_buffer_create(vertices_count * sizeof(GFX_Vertex2D), vertices);
  GFX_IndexBuffer ibo = gfx_index_buffer_create(indices_count, indices);

  gfx_vertex_array_add_buffer(&vao, &vbo, _gfx_vertex2d_layout());

  GFX_Mesh2D mesh = {
      .vertices = vertices,
      .indices = indices,
      ._vao = vao,
      ._vbo = vbo,
      ._ibo = ibo,
  };
  return mesh;
}

void gfx_mesh2d_destroy(const GFX_Mesh2D *mesh) {
  gfx_vertex_array_destroy(&mesh->_vao);
  gfx_vertex_buffer_destroy(&mesh->_vbo);
  gfx_index_buffer_destroy(&mesh->_ibo);
}

void gfx_mesh2d_draw(const GFX_Mesh2D *mesh, const GFX_Shader *shader,
                     mat4 trasform_model, mat4 trasform_view,
                     mat4 trasform_projection) {
  gfx_shader_bind(shader);

  gfx_shader_uniform_set_mat4(shader, "t_projection", false,
                              trasform_projection);
  gfx_shader_uniform_set_mat4(shader, "t_view", false, trasform_view);
  gfx_shader_uniform_set_mat4(shader, "t_model", false, trasform_model);

  gfx_vertex_array_bind(&mesh->_vao);
  gfx_index_buffer_bind(&mesh->_ibo);
  glDrawElements(GL_TRIANGLES, mesh->_ibo.count, GL_UNSIGNED_INT, NULL);
}

GFX_Mesh2D gfx_mesh2d_shape_quad_create(float_t width, float_t height) {
  GFX_Vertex2D vertices[4] = {
      gfx_vertex2d_create(-width / 2.0, -height / 2.0, 0.0, 0.0),
      gfx_vertex2d_create(-width / 2.0, +height / 2.0, 0.0, 1.0),
      gfx_vertex2d_create(+width / 2.0, -height / 2.0, 1.0, 0.0),
      gfx_vertex2d_create(+width / 2.0, +height / 2.0, 1.0, 1.0),
  };
  u_int32_t indices[6] = {0, 1, 2, 2, 3, 1};

  return gfx_mesh2d_create(4, vertices, 6, indices);
}
