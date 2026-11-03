#include <gfx/transform.h>

ECS_COMPONENT_DECLARE(GFX_Transform);

void GfxTransformImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxTransform);

  ECS_COMPONENT_DEFINE(world, GFX_Transform);
}
