#include "../include/gfx/gfx.h"

void GfxTransformImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxTransform);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT(world, GFX_Transform);
}
