#include <assert.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include <gfx/engine.h>

GFX_Error gfx_init(const GFX_ProjectSettings *settings, ecs_world_t *world) {
  // Start graphics GLFW
  GFX_Error err = gfx_graphics_init();
  if (err) {
    return err;
  }

  // Create the window
  GFX_Window *win = gfx_window_init(
      settings->window_width, settings->window_height, settings->window_name);
  if (!win) {
    return GFX_ERR_GLFW;
  }
  // Enable features
  for (int i = 0; i < settings->graphics_features_count; ++i) {
    gfx_graphics_enable(settings->graphics_features[i]);
  }
  // Set clear
  gfx_color_buffer_set_clear(settings->clear_color);

  // Import engine and renderer
  ECS_IMPORT(world, GfxEngine);
  ECS_IMPORT(world, GfxRenderer);

  // Add window surface entity
  ecs_entity_t surface_id =
      ecs_insert(world, ecs_value(GFX_Surface, {.window = win}));

  // Add uniforms entity
  ecs_entity_t uniforms_id = ecs_insert(
      world, ecs_value(GFX_Uniforms, {.t_camera_mvp = GLM_MAT4_IDENTITY_INIT}));

  // Add engine singleton
  ecs_singleton_set(world, GFX_Engine,
                    {
                        .surface_id = surface_id,
                        .uniforms_id = uniforms_id,
                    });

  return GFX_ERR_OK;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static void _gfx_run_frame(void *world) {
  if (!ecs_progress(world, 0)) {
    emscripten_cancel_main_loop();
  }
}
#endif

// Run the main loop until the window should close. In the browser the frames
// are driven by requestAnimationFrame and this call does not return.
void gfx_run(ecs_world_t *world) {
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(_gfx_run_frame, world, 0, true);
#else
  while (ecs_progress(world, 0)) {
  }
#endif
}

void gfx_terminate(ecs_world_t *world) {
  // Stop ecs
  if (world) {
    ecs_fini(world);
    fprintf(stderr, "[i] system: ecs world terminated\n");
  }
  // Stop graphics
  gfx_graphics_terminate();
  fprintf(stderr, "[i] system: graphics terminated\n");
}
