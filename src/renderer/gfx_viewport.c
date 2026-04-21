#include "../../include/gfx/gfx.h"

void _gfx_viewport_write_camera(
    const ecs_world_t *world
) {
}

void GfxViewportImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxViewport);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Viewport);
}
