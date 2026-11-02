#include <gfx/gfx.h>

// Uniform of the unlit material
typedef struct {
  vec4 albedo;
} UnlitUniform;

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }
  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");
  if (win == NULL) {
    gfx_graphics_terminate();
    return 1;
  }

  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING);
  gfx_graphics_enable(GFX_GRAPHICS_FEATURE_CULL_BACK);

  GFX_Texture2D tex =
      gfx_texture2d_load("./assets/textures/uv_checker_bw.png",
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

  mat4 view_projection;
  glm_mat4_mul(transform_projection, transform_view, view_projection);
  vec4 full_window = {0.0f, 0.0f, 1.0f, 1.0f};

  // Unlit material: albedo color * albedo texture
  GFX_MaterialLayout unlit =
      gfx_material_layout_create(sizeof(UnlitUniform), 1);
  UnlitUniform uniform = {.albedo = {0.5, 0.5, 0.0, 1.0}};
  const GFX_Texture2D *textures[] = {&tex};
  GFX_Material material = gfx_material_create(&unlit, &uniform, textures);

  GFX_Shader shader =
      gfx_shader_create("./assets/shaders/mesh3d_std_unlit.wgsl");
  GFX_Pipeline pipeline =
      gfx_pipeline_create(&shader, gfx_geometry3d_layout(), &unlit);

  while (!gfx_window_should_close(win)) {
    gfx_frame_begin(win);
    gfx_frame_view_begin(NULL, full_window, view_projection);

    gfx_pipeline_bind(&pipeline);
    gfx_material_bind(&material);
    gfx_draw_set_model(transform_model);
    gfx_geometry3d_draw(&geometry);

    gfx_frame_end(win);
    gfx_window_events_poll();
  }

  gfx_pipeline_destroy(&pipeline);
  gfx_shader_destroy(&shader);
  gfx_material_destroy(&material);
  gfx_material_layout_destroy(&unlit);
  gfx_texture2d_destroy(&tex);
  gfx_geometry3d_destroy(&geometry);
  gfx_window_destroy(win);
  gfx_graphics_terminate();
  return 0;
}
