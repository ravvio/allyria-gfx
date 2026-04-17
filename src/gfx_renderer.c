#include "../include/gfx/gfx.h"

// Rendering task
void _gfx_renderer_render(ecs_iter_t *it) {
  // Load
  GFX_Engine *engine = ecs_field(it, GFX_Engine, 0);
  GFX_Surface *surface = engine->surface;
}

void GfxRendererImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxRenderer);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Uniform);
  ECS_COMPONENT_DEFINE(world, GFX_Renderer);

  ECS_SYSTEM(world, _gfx_renderer_render, EcsOnStore, GFX_Engine);
  ecs_system(world, {
                        .entity = ecs_id(_gfx_renderer_render),
                        .immediate = true,
                    });
}
