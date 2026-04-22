#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION

#include "../include/gfx/gfx.h"

GFX_Error gfx_init(const GFX_ProjectSettings *settings, ecs_world_t *world) {
  // Start graphics GLFW
  GFX_Error err = gfx_graphics_init();
  if (err) {
    return err;
  }

  // Create the window
  GFX_Window *win = gfx_window_init(
      settings->window_width, settings->window_height, settings->window_name);
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

  // Add engine singleton
  ecs_singleton_set(world, GFX_Engine,
                    {
                        .surface_id = surface_id,
                    });

  return GFX_ERR_OK;
}

void gfx_terminate() {
  // Stop graphics
  gfx_graphics_terminate();
}
