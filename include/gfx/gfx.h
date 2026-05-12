#ifndef GFX_DECL
#define GFX_DECL

// Include libraries
#include "gfx_common.h"
#include "graphics/gfx_graphics.h"
#include "sprite/gfx_sprite.h"

// Setup

typedef struct {
  const char *window_name;
  int window_width;
  int window_height;
  enum GFX_Graphics_Feature *graphics_features;
  int graphics_features_count;
  GFX_Color_RGBA clear_color;
} GFX_ProjectSettings;

GFX_Error gfx_init(const GFX_ProjectSettings *settings, ecs_world_t *world);
void gfx_terminate(ecs_world_t *world);

// Transform

typedef mat4 GFX_Transform;
extern ECS_COMPONENT_DECLARE(GFX_Transform);

void GfxTransformImport(ecs_world_t *world);

// Camera

typedef struct {
  mat4 projection;
  mat4 view;
  mat4 model;
} GFX_Camera;
extern ECS_COMPONENT_DECLARE(GFX_Camera);

typedef struct {
  float aspect;
} GFX_CameraOrtho;
extern ECS_COMPONENT_DECLARE(GFX_CameraOrtho);

typedef struct {
  float ratio;
} GFX_CameraPixel;
extern ECS_COMPONENT_DECLARE(GFX_CameraPixel);

GFX_Camera gfx_camera_create_ortho(float aspect);

void GfxCameraImport(ecs_world_t *world);

// Surface

typedef struct {
  GFX_Window *window;
  int width;
  int height;
  float scale_x;
  float scale_y;
  int true_width;
  int true_height;
} GFX_Surface;
extern ECS_COMPONENT_DECLARE(GFX_Surface);
void gfx_surface_register(ecs_world_t *world);

void gfx_surface_sync(GFX_Surface *surface);

// Renderer

extern ECS_TAG_DECLARE(GfxPreRender);
extern ECS_TAG_DECLARE(GfxStartRender);
extern ECS_TAG_DECLARE(GfxRenderObjects);
extern ECS_TAG_DECLARE(GfxRenderGizmos);
extern ECS_TAG_DECLARE(GfxRenderGUI);
extern ECS_TAG_DECLARE(GfxEndRender);
extern ECS_TAG_DECLARE(GfxPostRender);

typedef struct {
  // The camera projection * view * model
  mat4 t_camera_mvp;
} GFX_Uniforms;
extern ECS_COMPONENT_DECLARE(GFX_Uniforms);

void GfxRendererImport(ecs_world_t *world);

// Viewport

typedef struct {
} GFX_Viewport;
extern ECS_COMPONENT_DECLARE(GFX_Viewport);

// Engine

typedef struct {
  ecs_entity_t surface_id;
  ecs_entity_t uniforms_id;
  ecs_entity_t main_camera_id;
} GFX_Engine;
extern ECS_COMPONENT_DECLARE(GFX_Engine);

void GfxEngineImport(ecs_world_t *world);

#endif
