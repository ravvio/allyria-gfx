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

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_geometry2d_draw(&geometry, &shader, transform_projection, transform_view, transform_model);

    gfx_window_swap_buffers(win);
    gfx_window_events_poll();
  }

  gfx_geometry2d_destroy(&geometry);
  gfx_graphics_terminate();
  return 0;
}
