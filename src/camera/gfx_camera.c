#include <gfx/camera.h>
#include <gfx/engine.h>
#include <gfx/transform.h>

ECS_COMPONENT_DECLARE(GFX_Camera);
ECS_COMPONENT_DECLARE(GFX_CameraOrtho);
ECS_COMPONENT_DECLARE(GFX_CameraPerspective);

// The world moves opposite to the camera
void _gfx_sys_camera_sync_transform(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_Transform *t = ecs_field(it, GFX_Transform, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_mat4_inv(t[i], cam[i].view);
  }
}

// Perspective projection of a square view
void _gfx_sys_camera_perspective_project(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_CameraPerspective *p = ecs_field(it, GFX_CameraPerspective, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_perspective(p[i].fov, 1.0f, p[i].near, p[i].far, cam[i].projection);
  }
}

// Orthographic projection of a square view
void _gfx_sys_camera_ortho_project(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_CameraOrtho *ortho = ecs_field(it, GFX_CameraOrtho, 1);

  for (int i = 0; i < it->count; ++i) {
    float s = ortho[i].size;
    glm_ortho(-s, s, -s, s, -100.0f, 100.0f, cam[i].projection);
  }
}

// View-projection of camera seen through a viewport of this aspect ratio
void gfx_camera_view_projection(GFX_Camera camera, float aspect, mat4 dest) {
  // Stretch the square projection to the viewport
  mat4 fit = GLM_MAT4_IDENTITY_INIT;
  switch (camera.scaling_mode) {
  case GFX_CAMERA_SCALING_MODE_FIT_HEIGHT:
    fit[0][0] = 1.0f / aspect;
    break;
  case GFX_CAMERA_SCALING_MODE_FIT_WIDTH:
    fit[1][1] = aspect;
    break;
  }

  glm_mat4_mul(fit, camera.projection, dest);
  glm_mat4_mul(dest, camera.view, dest);
}

void GfxCameraImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxCamera);

  ECS_COMPONENT_DEFINE(world, GFX_Camera);
  ECS_COMPONENT_DEFINE(world, GFX_CameraOrtho);
  ECS_COMPONENT_DEFINE(world, GFX_CameraPerspective);

  // Independent of each other, so they can run in any order
  ECS_SYSTEM(world, _gfx_sys_camera_sync_transform, GfxPreRender, GFX_Camera,
             GFX_Transform);
  ECS_SYSTEM(world, _gfx_sys_camera_perspective_project, GfxPreRender,
             GFX_Camera, GFX_CameraPerspective);
  ECS_SYSTEM(world, _gfx_sys_camera_ortho_project, GfxPreRender, GFX_Camera,
             GFX_CameraOrtho);
}
