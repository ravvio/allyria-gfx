#include "../include/gfx/gfx.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");
  gfx_enable(GFX_FEATURE_DEPTH_TESTING);
  gfx_enable(GFX_FEATURE_CULL_BACK);

  GFX_Mesh3D mesh = gfx_mesh3d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh_3d.vert",
                                        "./assets/shaders/mesh_3d.frag");
  mat4 transform_model = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_view = GLM_MAT4_IDENTITY_INIT;
  mat4 transform_projection = GLM_MAT4_IDENTITY_INIT;

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_mesh3d_draw(&mesh, &shader, transform_projection, transform_view, transform_model);

    gfx_window_swap_buffers(win);
    gfx_events_poll();
  }

  gfx_mesh3d_destroy(&mesh);
  gfx_terminate();
  return 0;
}
