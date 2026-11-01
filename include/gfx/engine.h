#ifndef GFX_ENGINE_DECL
#define GFX_ENGINE_DECL

#include <flecs.h>
#include <gfx/core.h>
#include <gfx/graphics.h>
#include <gfx/window.h>

// Setup

typedef struct {
  const char *window_name;
  int window_width;
  int window_height;
  enum GFX_GraphicsFeature *graphics_features;
  int graphics_features_count;
  GFX_ColorRGBA clear_color;
} GFX_ProjectSettings;

GFX_Error gfx_init(const GFX_ProjectSettings *settings, ecs_world_t *world);
void gfx_terminate(ecs_world_t *world);
void gfx_run(ecs_world_t *world);

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
  // The camera projection * view * inverse(transform)
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
