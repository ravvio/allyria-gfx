#ifndef GFX_ENGINE_DECL
#define GFX_ENGINE_DECL

#include <flecs.h>
#include <gfx/core.h>
#include <gfx/graphics.h>
#include <gfx/window.h>

// Setup

typedef struct {
  enum GFX_GraphicsFeature *graphics_features;
  int graphics_features_count;
  GFX_ColorRGBA clear_color;
} GFX_ProjectSettings;

GFX_Error gfx_init(const GFX_ProjectSettings *settings, ecs_world_t *world);
void gfx_terminate(ecs_world_t *world);
void gfx_run(ecs_world_t *world);

// Surface

typedef struct {
  const char *title;
  int width;
  int height;

  GFX_Window *window;
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

void GfxRendererImport(ecs_world_t *world);

// Viewport

// What camera sees, drawn into a rect of the window by the renderers that are
// children of the viewport
typedef struct {
  ecs_entity_t camera;
  // {x, y, width, height} in fractions of the window, from the top left
  vec4 rect;
  // Higher orders draw later, on top
  int32_t order;
} GFX_Viewport;
extern ECS_COMPONENT_DECLARE(GFX_Viewport);

// A renderer, draws into the viewport it is a child of
typedef struct {
  void (*draw)(ecs_world_t *world, ecs_entity_t batch,
               const GFX_Viewport *viewport);
} GFX_RenderBatch;
extern ECS_COMPONENT_DECLARE(GFX_RenderBatch);

void gfx_viewport_register(ecs_world_t *world);

// Engine

void GfxEngineImport(ecs_world_t *world);

#endif
