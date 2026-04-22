#include "../include/gfx/gfx.h"

ECS_COMPONENT_DECLARE(GFX_Transform);

void GfxTransformImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxTransform);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Transform);
}
