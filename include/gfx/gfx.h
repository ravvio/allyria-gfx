// Include libraries
#include "gfx_common.h"
#include "graphics/gfx_graphics.h"

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
void gfx_terminate(void);

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

void gfx_camera_create_ortho(float aspect);

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

typedef struct {
  // The camera model * view * projection
  mat4 camera_mvp;
} GFX_Uniform;
extern ECS_COMPONENT_DECLARE(GFX_Uniform);

void GfxRendererImport(ecs_world_t *world);

// Viewport

typedef struct {
} GFX_Viewport;
extern ECS_COMPONENT_DECLARE(GFX_Viewport);

// Engine

typedef struct {
  ecs_entity_t surface_id;
} GFX_Engine;
extern ECS_COMPONENT_DECLARE(GFX_Engine);

void GfxEngineImport(ecs_world_t *world);
