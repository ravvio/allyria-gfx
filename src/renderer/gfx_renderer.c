#include "../../include/gfx/gfx.h"

// Rendering system
void _gfx_sys_render(ecs_iter_t *it) {
  #ifdef GFX_VERBOSE
  printf("[i] render start | dt %f\n", it->delta_time);
  #endif

  // Load and sync surface
  const GFX_Engine *engine = ecs_singleton_get(it->world, GFX_Engine);
  GFX_Surface *surface = ecs_get_mut(it->world, engine->surface_id, GFX_Surface);
  gfx_surface_sync(surface);

  // End render
  gfx_window_swap_buffers(surface->window);
  gfx_window_events_poll();
}

void GfxRendererImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxRenderer);
  ecs_set_name_prefix(world, "Gfx");

  gfx_surface_register(world);
  ECS_COMPONENT_DEFINE(world, GFX_Uniform);

  ECS_SYSTEM(world, _gfx_sys_render, EcsOnStore, GFX_Engine);
  ecs_system(world, {
                        .entity = ecs_id(_gfx_sys_render),
                        .immediate = true,
                    });
}
