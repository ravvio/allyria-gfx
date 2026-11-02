#include <gfx/engine.h>
#include <stdio.h>

// Close the windows that should close, quit when none is left
void _gfx_sys_should_stop(ecs_iter_t *it) {
  int open = 0;
  ecs_iter_t s = ecs_each(it->world, GFX_Surface);
  while (ecs_each_next(&s)) {
    GFX_Surface *surfaces = ecs_field(&s, GFX_Surface, 0);
    for (int i = 0; i < s.count; ++i) {
      if (surfaces[i].window && !gfx_window_should_close(surfaces[i].window)) {
        open++;
      } else {
        ecs_delete(it->world, s.entities[i]);
      }
    }
  }
  if (open == 0) {
    fprintf(stderr, "[i] system: no window left, terminating engine\n");
    ecs_quit(it->world);
  }
}

void GfxEngineImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxEngine);
  ecs_set_name_prefix(world, "Gfx");

  ECS_SYSTEM(world, _gfx_sys_should_stop, EcsPreUpdate, 0);
}
