#include <gfx/engine.h>
#include <stdio.h>

// Quit ecs if the window should close or never opened
void _gfx_sys_should_stop(ecs_iter_t *it) {
  const GFX_Surface *surface = ecs_field(it, GFX_Surface, 0);
  if (!surface->window || gfx_window_should_close(surface->window)) {
    fprintf(stderr, "[i] system: window should close, terminating engine\n");
    ecs_quit(it->world);
  }
}

void GfxEngineImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxEngine);
  ecs_set_name_prefix(world, "Gfx");

  ECS_SYSTEM(world, _gfx_sys_should_stop, EcsPreUpdate, GFX_Surface);
}
