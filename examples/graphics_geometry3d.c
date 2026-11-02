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
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh3d.wgsl", 0, 0);
  GFX_Pipeline pipeline = gfx_pipeline_create(&shader, gfx_geometry3d_layout());

  // Identity camera, the quad covers the middle of the screen
  mat4 identity = GLM_MAT4_IDENTITY_INIT;
  vec4 full_window = {0.0f, 0.0f, 1.0f, 1.0f};

  while (!gfx_window_should_close(win)) {
    gfx_frame_begin();
    gfx_frame_window_begin(win);
    gfx_frame_view_begin(win, full_window, identity);

    gfx_pipeline_bind(&pipeline);
    gfx_draw_set_model(identity);
    gfx_geometry3d_draw(&geometry);

    gfx_frame_end();
    gfx_window_events_poll();
  }

  gfx_pipeline_destroy(&pipeline);
  gfx_shader_destroy(&shader);
  gfx_geometry3d_destroy(&geometry);
  gfx_window_destroy(win);
  gfx_graphics_terminate();
  return 0;
}
