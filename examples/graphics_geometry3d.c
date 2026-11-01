#include <gfx/gfx.h>

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Geometry3D Example");
  if (win == NULL) {
    gfx_graphics_terminate();
    return 1;
  }
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING);
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_CULL_BACK);

  GFX_Geometry3D geometry = gfx_geometry3d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh3d.wgsl");
  GFX_Pipeline pipeline =
      gfx_pipeline_create(&shader, gfx_geometry3d_layout(), NULL);

  // Identity camera, the quad covers the middle of the screen
  mat4 model = GLM_MAT4_IDENTITY_INIT;

  while (!gfx_window_should_close(win)) {
    gfx_frame_begin(win);

    gfx_pipeline_bind(&pipeline);
    gfx_draw_set_model(model);
    gfx_geometry3d_draw(&geometry);

    gfx_frame_end(win);
    gfx_window_events_poll();
  }

  gfx_pipeline_destroy(&pipeline);
  gfx_shader_destroy(&shader);
  gfx_geometry3d_destroy(&geometry);
  gfx_window_destroy(win);
  gfx_graphics_terminate();
  return 0;
}
