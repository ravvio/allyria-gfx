#include "../include/gfx/gfx.h"
#include <assert.h>
#include <stdio.h>

ECS_COMPONENT_DECLARE(GFX_Engine);

// Quit ecs if the engine window should close
void _gfx_sys_should_stop(ecs_iter_t *it) {
  const GFX_Engine *engine = ecs_singleton_get(it->world, GFX_Engine);
  assert(engine != NULL);

  GFX_Surface *surface =
      ecs_get_mut(it->world, engine->surface_id, GFX_Surface);
  assert(surface != NULL);

  if (gfx_window_should_close(surface->window)) {
      fprintf(stderr, "[i] system: Engine window should close, terminating engine\n");
    ecs_quit(it->world);
  }
}

void GfxEngineImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxEngine);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Engine);

  ECS_SYSTEM(world, _gfx_sys_should_stop, EcsPreUpdate, GFX_Engine);
}
