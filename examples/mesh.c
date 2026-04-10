#include "../include/gfx/gfx.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");
  assert(win != NULL);

  GFX_Mesh3D quad = gfx_mesh3d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/shape_3d.vert",
                                        "./assets/shaders/shape_3d.frag");
  GFX_Transform transform_model = GLM_MAT4_IDENTITY_INIT;
  GFX_Transform transform_view = GLM_MAT4_IDENTITY_INIT;
  GFX_Transform transform_projection = GLM_MAT4_IDENTITY_INIT;

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_mesh3d_draw(&quad, &shader, transform_model, transform_view, transform_projection);

    gfx_window_swap_buffers(win);
    gfx_events_poll();
  }

  gfx_mesh3d_destroy(&quad);
  gfx_terminate();
  return 0;
}
