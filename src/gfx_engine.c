#include "../include/gfx/gfx.h"

ECS_COMPONENT_DECLARE(GFX_Engine);

void GfxEngineImport(ecs_world_t *world) {
    ECS_MODULE(world, GfxEngine);
    ecs_set_name_prefix(world, "Gfx");

    ECS_COMPONENT_DEFINE(world, GFX_Engine);
    ecs_singleton_add(world, GFX_Engine);
}
