#ifndef GFX_CAMERA_DECL
#define GFX_CAMERA_DECL

#include <flecs.h>
#include <gfx/core.h>

enum GFX_CameraScalingMode {
  GFX_CAMERA_SCALING_MODE_FIT_HEIGHT,
  GFX_CAMERA_SCALING_MODE_FIT_WIDTH,
};

typedef struct {
  enum GFX_CameraScalingMode scaling_mode;
  mat4 projection;
  mat4 view;
} GFX_Camera;
extern ECS_COMPONENT_DECLARE(GFX_Camera);

typedef struct {
  // Half the visible extent along the fixed axis in world units
  float size;
} GFX_CameraOrtho;
extern ECS_COMPONENT_DECLARE(GFX_CameraOrtho);

typedef struct {
  // Field of view along the fixed axis in radians
  float fov;
  float near;
  float far;
} GFX_CameraPerspective;
extern ECS_COMPONENT_DECLARE(GFX_CameraPerspective);

// View-projection of camera seen through a viewport of this aspect ratio
void gfx_camera_view_projection(GFX_Camera camera, float aspect, mat4 dest);

void GfxCameraImport(ecs_world_t *world);

#endif
