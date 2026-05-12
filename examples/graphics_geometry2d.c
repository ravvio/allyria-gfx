#include "../include/gfx/graphics/gfx_graphics.h"

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Geometry2D Example");

  GFX_Geometry2D geometry = gfx_geometry2d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh2d.vert",
                                        "./assets/shaders/mesh2d.frag");
  mat4 transform_model = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_view = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_projection = GLM_MAT4_IDENTITY_INIT;
  glm_translate_z(transform_view, -1);

  gfx_shader_uniform_set_mat4(&shader, "t_projection", false, transform_projection);
  gfx_shader_uniform_set_mat4(&shader, "t_view", false, transform_view);
  gfx_shader_uniform_set_mat4(&shader, "t_model", false, transform_model);

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_shader_bind(&shader);
    gfx_geometry2d_draw(&geometry);

    gfx_window_swap_buffers(win);
    gfx_window_events_poll();
  }

  gfx_geometry2d_destroy(&geometry);
  gfx_graphics_terminate();
  return 0;
}
