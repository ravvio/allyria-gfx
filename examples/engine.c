#include "../include/gfx/gfx.h"

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }
  GFX_Window *win = gfx_window_init(800, 600, "Mesh Example");
  gfx_enable(GFX_FEATURE_DEPTH_TESTING);
  gfx_enable(GFX_FEATURE_CULL_BACK);

  ecs_world_t *world = ecs_init();
  ECS_IMPORT(world, GfxEngine);
  ECS_IMPORT(world, GfxRenderer);
  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);

  ecs_entity_t surface_id = ecs_insert(world, ecs_value(GFX_Surface, {.window = win}));

  GFX_Engine *engine = ecs_singleton_get_mut(world, GFX_Engine);
  engine->surface_id = surface_id;

  while (!gfx_window_should_close(win) && ecs_progress(world, 0)) {
  }

  gfx_terminate();
  return 0;
}
