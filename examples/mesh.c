#include "../include/gfx/gfx.h"
#include <OpenGL/gl.h>
#include <stdio.h>

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");

  GFX_Mesh3D quad = gfx_mesh3d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/shape_3d.vert",
                                        "./assets/shaders/shape_3d.frag");

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_mesh3d_draw(&quad, &shader);

    gfx_window_swap_buffers(win);
    gfx_events_poll();
  }

  gfx_mesh3d_destroy(&quad);
  gfx_terminate();
  return 0;
}
