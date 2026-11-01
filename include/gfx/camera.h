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
  mat4 model;
} GFX_Camera;
extern ECS_COMPONENT_DECLARE(GFX_Camera);

typedef struct {
  // Half the visible extent along the fixed axis in world units
  float size;
} GFX_CameraOrtho;
extern ECS_COMPONENT_DECLARE(GFX_CameraOrtho);

typedef struct {
  float ratio;
} GFX_CameraPixel;
extern ECS_COMPONENT_DECLARE(GFX_CameraPixel);

void GfxCameraImport(ecs_world_t *world);

#endif
