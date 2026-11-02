#include <gfx/gfx.h>

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Geometry2D Example");
  if (win == NULL) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Geometry2D geometry = gfx_geometry2d_shape_quad_create(1.0, 1.0);
  GFX_Shader shader = gfx_shader_create("./assets/shaders/mesh2d.wgsl");
  GFX_Pipeline pipeline =
      gfx_pipeline_create(&shader, gfx_geometry2d_layout(), NULL);

  mat4 view_projection;
  glm_ortho_default(800.0f / 600.0f, view_projection);
  vec4 full_window = {0.0f, 0.0f, 1.0f, 1.0f};

  // Three quads sharing the geometry, each draw gets its own model
  mat4 models[3];
  for (int i = 0; i < 3; i++) {
    glm_mat4_identity(models[i]);
    glm_translate_x(models[i], (i - 1) * 0.8f);
    glm_rotate_z(models[i], i * 0.4f, models[i]);
    glm_scale_uni(models[i], 0.5f);
  }

  while (!gfx_window_should_close(win)) {
    gfx_frame_begin(win);
    gfx_frame_view_begin(NULL, full_window, view_projection);

    gfx_pipeline_bind(&pipeline);
    for (int i = 0; i < 3; i++) {
      gfx_draw_set_model(models[i]);
      gfx_geometry2d_draw(&geometry);
    }

    gfx_frame_end(win);
    gfx_window_events_poll();
  }

  gfx_pipeline_destroy(&pipeline);
  gfx_shader_destroy(&shader);
  gfx_geometry2d_destroy(&geometry);
  gfx_window_destroy(win);
  gfx_graphics_terminate();
  return 0;
}
