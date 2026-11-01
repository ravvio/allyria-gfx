#ifndef GFX_TRANSFORM_DECL
#define GFX_TRANSFORM_DECL

#include <flecs.h>
#include <gfx/core.h>

typedef mat4 GFX_Transform;
extern ECS_COMPONENT_DECLARE(GFX_Transform);

void GfxTransformImport(ecs_world_t *world);

#endif
