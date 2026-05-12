#include "../include/gfx/graphics/gfx_graphics.h"

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Geometry3D Example");
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING);
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_CULL_BACK);

  GFX_Geometry3D geometry = gfx_geometry3d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh3d.vert",
                                        "./assets/shaders/mesh3d.frag");
  mat4 transform_model = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_view = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_projection = GLM_MAT4_IDENTITY_INIT;

  gfx_shader_uniform_set_mat4(&shader, "t_projection", false, transform_projection);
  gfx_shader_uniform_set_mat4(&shader, "t_view", false, transform_view);
  gfx_shader_uniform_set_mat4(&shader, "t_model", false, transform_model);

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_shader_bind(&shader);
    gfx_geometry3d_draw(&geometry);

    gfx_window_swap_buffers(win);
    gfx_window_events_poll();
  }

  gfx_geometry3d_destroy(&geometry);
  gfx_graphics_terminate();
  return 0;
}
