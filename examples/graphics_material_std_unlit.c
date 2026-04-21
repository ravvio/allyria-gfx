#include "../include/gfx/graphics/gfx_graphics.h"

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }
  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");

  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING);
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_CULL_BACK);

  GFX_Texture2D tex = gfx_texture2d_load(
      "./assets/textures/tiles.jpg", GFX_TEXTURE_TYPE_DIFFUSE,
      GFX_TEXTURE_FILTER_LINEAR, GFX_WRAP_REPEAT);

  GFX_Geometry3D geometry = gfx_geometry3d_shape_cuboid_create(1.0, 1.0, 1.0);

  mat4 transform_model = GLM_MAT4_IDENTITY_INIT;
  vec3 r = {1.0, 1.0, 0.0};
  glm_rotate(transform_model, glm_rad(50), r);

  mat4 transform_view = GLM_MAT4_IDENTITY_INIT;
  vec3 t = {0.0, 0.0, -5.0};
  glm_translate(transform_view, t);

  mat4 transform_projection = GLM_MAT4_IDENTITY_INIT;
  glm_perspective(glm_rad(45.0), 800.0 / 600.0, 0.1f, 100.0f,
                  transform_projection);

  // Mesh3D standard unlit shader
  GFX_Shader shader =
      gfx_shader_create("./assets/shaders/mesh3d_std_unlit.vert",
                        "./assets/shaders/mesh3d_std_unlit.frag");
  GFX_Color_RGBA albedo = { 0.5, 1.0, 0.0, 1.0 };
  gfx_shader_uniform_set_vec4(&shader, "material.albedo", albedo);
  // This is not needed as the default albedoMap is 0
  gfx_shader_uniform_set_int(&shader, "material.albedoMap", 0);

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_texture2d_bind(&tex);
    gfx_geometry3d_draw(&geometry, &shader, transform_projection, transform_view,
                    transform_model);

    gfx_window_swap_buffers(win);
    gfx_window_events_poll();
  }

  gfx_shader_destroy(&shader);
  gfx_texture2d_destroy(&tex);
  gfx_geometry3d_destroy(&geometry);
  gfx_graphics_terminate();
  return 0;
}
