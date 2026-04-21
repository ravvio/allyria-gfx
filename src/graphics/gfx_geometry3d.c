#include "../../include/gfx/graphics/gfx_gl.h"

static GFX_Boolean __gfx_vertex3d_layout_init = GFX_FALSE;
static GFX_VertexLayout __gfx_vertex3d_layout;

GFX_VertexLayout *_gfx_vertex3d_layout() {
  if (__gfx_vertex3d_layout_init == GFX_TRUE) {
    return &__gfx_vertex3d_layout;
  }

  __gfx_vertex3d_layout = gfx_vertex_layout_create();
  // Position
  gfx_vertex_layout_push_f32(&__gfx_vertex3d_layout, 3, false);
  // Normal
  gfx_vertex_layout_push_f32(&__gfx_vertex3d_layout, 3, false);
  // UV
  gfx_vertex_layout_push_f32(&__gfx_vertex3d_layout, 2, false);

  fprintf(stderr, "[i] vertex3D layout initialized\n");
  return &__gfx_vertex3d_layout;
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

GFX_Geometry3D gfx_geometry3d_create(u_int32_t vertices_count, GFX_Vertex3D *vertices,
                             u_int32_t indices_count, u_int32_t *indices) {
  GFX_VertexArray vao = gfx_vertex_array_create();
  GFX_VertexBuffer vbo =
      gfx_vertex_buffer_create(vertices_count * sizeof(GFX_Vertex3D), vertices);
  GFX_IndexBuffer ibo = gfx_index_buffer_create(indices_count, indices);

  gfx_vertex_array_add_buffer(&vao, &vbo, _gfx_vertex3d_layout());

  GFX_Geometry3D geometry = {
      .vertices = vertices,
      .indices = indices,
      ._vao = vao,
      ._vbo = vbo,
      ._ibo = ibo,
  };
  return geometry;
}

void gfx_geometry3d_destroy(const GFX_Geometry3D *geometry) {
  gfx_vertex_array_destroy(&geometry->_vao);
  gfx_vertex_buffer_destroy(&geometry->_vbo);
  gfx_index_buffer_destroy(&geometry->_ibo);
}

void gfx_geometry3d_draw(const GFX_Geometry3D *geometry, const GFX_Shader *shader,
                     mat4 trasform_projection, mat4 trasform_view,
                     mat4 trasform_model) {
  gfx_shader_bind(shader);

  gfx_shader_uniform_set_mat4(shader, "t_model", false, trasform_model);
  gfx_shader_uniform_set_mat4(shader, "t_view", false, trasform_view);
  gfx_shader_uniform_set_mat4(shader, "t_projection", false,
                              trasform_projection);

  gfx_vertex_array_bind(&geometry->_vao);
  gfx_index_buffer_bind(&geometry->_ibo);
  glDrawElements(GL_TRIANGLES, geometry->_ibo.count, GL_UNSIGNED_INT, NULL);
}

GFX_Geometry3D gfx_geometry3d_shape_quad_create(float_t width, float_t height) {
  GFX_Vertex3D vertices[4] = {
      // front
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, -1.0, 0.0,
                          1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, -1.0, 0.0,
                          0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, 0.0, 0.0, 0.0, -1.0, 1.0,
                          0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, 0.0, 0.0, 0.0, -1.0, 1.0,
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
                          -1.0, 0.0, 1.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, +depth / 2.0, 0.0, 0.0,
                          -1.0, 0.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, +depth / 2.0, 0.0, 0.0,
                          -1.0, 1.0, 0.0),
      gfx_vertex3d_create(+width / 2.0, +height / 2.0, +depth / 2.0, 0.0, 0.0,
                          -1.0, 1.0, 1.0),
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
                          +1.0, 0.0, 1.0),
      gfx_vertex3d_create(+width / 2.0, -height / 2.0, -depth / 2.0, 0.0, 0.0,
                          +1.0, 0.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, -height / 2.0, -depth / 2.0, 0.0, 0.0,
                          +1.0, 1.0, 0.0),
      gfx_vertex3d_create(-width / 2.0, +height / 2.0, -depth / 2.0, 0.0, 0.0,
                          +1.0, 1.0, 1.0),
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

void gfx_geometry3d_import(ecs_world_t *world) {
  ECS_MODULE(world, gfx_geometry3d);
  ecs_set_name_prefix(world, "gfx_");

  ECS_COMPONENT(world, GFX_Geometry3D);
}
