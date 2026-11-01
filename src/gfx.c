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

  // Enable features
  for (int i = 0; i < settings->graphics_features_count; ++i) {
    gfx_graphics_enable(settings->graphics_features[i]);
  }
  // Set clear
  gfx_color_buffer_set_clear(settings->clear_color);

  // Import renderer and engine
  ECS_IMPORT(world, GfxRenderer);
  ECS_IMPORT(world, GfxEngine);

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

// Run the main loop until the window closes. In the browser it never returns.
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
