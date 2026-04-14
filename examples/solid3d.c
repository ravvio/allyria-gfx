#include "../include/gfx/gfx.h"

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }
  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");
  gfx_enable(GFX_FEATURE_DEPTH_TESTING);
  gfx_enable(GFX_FEATURE_CULL_BACK);

  GFX_Shader shader = gfx_shader_create("./assets/shaders/solid_3d.vert",
                                        "./assets/shaders/solid_3d.frag");
  GFX_Texture2D tex = gfx_texture2d_load("./assets/textures/tiles.jpg", GFX_TEXTURE_TYPE_DIFFUSE, GFX_TEXTURE_FILTER_LINEAR, GFX_WRAP_REPEAT);

  GFX_Mesh3D mesh = gfx_mesh3d_shape_cuboid_create(1.0, 1.0, 1.0);

  GFX_Transform transform_model = GLM_MAT4_IDENTITY_INIT;
  vec3 r = {1.0, 1.0, 0.0};

  GFX_Transform transform_view = GLM_MAT4_IDENTITY_INIT;
  vec3 t = {0.0, 0.0, -5.0};
  glm_translate(transform_view, t);

  GFX_Transform transform_projection = GLM_MAT4_IDENTITY_INIT;
  glm_perspective(glm_rad(45.0), 800.0/600.0, 0.1f, 100.0f, transform_projection);

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    glm_rotate(transform_model, glm_rad(0.5), r);

    gfx_texture2d_bind(&tex);
    gfx_mesh3d_draw(&mesh, &shader, transform_projection, transform_view, transform_model);

    gfx_window_swap_buffers(win);
    gfx_events_poll();
  }

  gfx_shader_destroy(&shader);
  gfx_texture2d_destroy(&tex);
  gfx_mesh3d_destroy(&mesh);
  gfx_terminate();
  return 0;
}
