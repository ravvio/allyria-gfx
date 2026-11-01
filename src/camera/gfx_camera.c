#include <gfx/camera.h>
#include <gfx/engine.h>
#include <gfx/transform.h>

ECS_COMPONENT_DECLARE(GFX_Camera);
ECS_COMPONENT_DECLARE(GFX_CameraOrtho);
ECS_COMPONENT_DECLARE(GFX_CameraPixel);

void _gfx_sys_camera_sync_transform(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_Transform *t = ecs_field(it, GFX_Transform, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_mat4_copy(t[i], cam[i].model);
  }
}

// Sync ratio of pixel cameras (pixels per world unit)
void _gfx_sys_camera_pixel_sync(ecs_iter_t *it) {
  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_CameraPixel *pixel = ecs_field(it, GFX_CameraPixel, 1);

  for (int i = 0; i < it->count; ++i) {
    glm_mat4_identity(cam[i].view);
    glm_scale_uni(cam[i].view, 1.0f / pixel[i].ratio);
  }
}

// Derive orthographic camera projection from the surface aspect ratio
void _gfx_sys_camera_ortho_project(ecs_iter_t *it) {
  const GFX_Engine *engine = ecs_singleton_get(it->world, GFX_Engine);
  const GFX_Surface *surface =
      ecs_get(it->world, engine->surface_id, GFX_Surface);

  // Minimized
  if (surface->true_height == 0) {
    return;
  }
  float aspect = (float)surface->true_width / surface->true_height;

  GFX_Camera *cam = ecs_field(it, GFX_Camera, 0);
  GFX_CameraOrtho *ortho = ecs_field(it, GFX_CameraOrtho, 1);

  for (int i = 0; i < it->count; ++i) {
    float s = ortho[i].size;
    switch (cam[i].scaling_mode) {
    // glm_ortho(left, right, bottom, top, near, far)
    case GFX_CAMERA_SCALING_MODE_FIT_HEIGHT:
      glm_ortho(-s * aspect, s * aspect, -s, s, -100.0f, 100.0f,
                cam[i].projection);
      break;
    case GFX_CAMERA_SCALING_MODE_FIT_WIDTH:
      glm_ortho(-s, s, -s / aspect, s / aspect, -100.0f, 100.0f,
                cam[i].projection);
      break;
    }
  }
}

void GfxCameraImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxCamera);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Camera);
  ECS_COMPONENT_DEFINE(world, GFX_CameraOrtho);
  ECS_COMPONENT_DEFINE(world, GFX_CameraPixel);

  // Render preparation, model, view and projection are independent so these
  // can run in any order
  ECS_SYSTEM(world, _gfx_sys_camera_sync_transform, GfxPreRender, GFX_Camera,
             GFX_Transform);
  ECS_SYSTEM(world, _gfx_sys_camera_pixel_sync, GfxPreRender, GFX_Camera,
             GFX_CameraPixel);
  ECS_SYSTEM(world, _gfx_sys_camera_ortho_project, GfxPreRender, GFX_Camera,
             GFX_CameraOrtho);
}
