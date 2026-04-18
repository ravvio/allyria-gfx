#include "../include/gfx/gfx.h"

ECS_COMPONENT_DECLARE(GFX_Camera);

void gfx_camera_create_ortho(float aspect) {
  GFX_Camera cam = {
      .projection = GLM_MAT4_IDENTITY_INIT,
      .view = GLM_MAT4_IDENTITY_INIT,
  };

  glm_ortho_default(aspect, cam.projection);
}

void _gfx_camera_sync(ecs_iter_t *it) {
  GFX_Transform *t = ecs_field(it, GFX_Transform, 0);
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_mat4_copy(t[i], cam[i].model);
  }
}

void GfxCameraImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxCamera);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Camera);

  ECS_SYSTEM(world, _gfx_camera_sync, EcsPostUpdate, GFX_Transform, GFX_Camera);
}
