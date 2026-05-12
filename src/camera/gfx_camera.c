#include "../../include/gfx/gfx.h"

ECS_COMPONENT_DECLARE(GFX_Camera);
ECS_COMPONENT_DECLARE(GFX_CameraOrtho);

GFX_Camera gfx_camera_create_ortho(float aspect) {
  GFX_Camera cam = {
      .projection = GLM_MAT4_IDENTITY_INIT,
      .view = GLM_MAT4_IDENTITY_INIT,
      .model = GLM_MAT4_IDENTITY_INIT,
  };

  glm_ortho_default(aspect, cam.projection);
  return cam;
}

void _gfx_sys_camera_sync_ortho(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_CameraOrtho *ortho = ecs_field(it, GFX_CameraOrtho, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_ortho_default(ortho[i].aspect, cam[i].projection);
  }
}

void _gfx_sys_camera_sync_transform(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_Transform *t = ecs_field(it, GFX_Transform, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_mat4_copy(t[i], cam[i].model);
  }
}

void GfxCameraImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxCamera);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Camera);
  ECS_COMPONENT_DEFINE(world, GFX_CameraOrtho);

  ECS_SYSTEM(world, _gfx_sys_camera_sync_ortho, EcsPostUpdate, GFX_Camera,
             GFX_CameraOrtho);
  ECS_SYSTEM(world, _gfx_sys_camera_sync_transform, EcsPostUpdate, GFX_Camera,
             GFX_Transform);
}
