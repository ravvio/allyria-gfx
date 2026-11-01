#include <gfx/camera.h>
#include <gfx/engine.h>
#include <gfx/render.h>

ECS_COMPONENT_DECLARE(GFX_Viewport);
ECS_COMPONENT_DECLARE(GFX_RenderBatch);

static int _gfx_viewport_compare(ecs_entity_t e1, const void *p1,
                                 ecs_entity_t e2, const void *p2) {
  (void)e1, (void)e2;
  const GFX_Viewport *a = p1;
  const GFX_Viewport *b = p2;
  return (a->order > b->order) - (a->order < b->order);
}

// Draw each viewport with its renderers, in order
void _gfx_sys_viewport_draw(ecs_iter_t *it) {
  GFX_Viewport *viewports = ecs_field(it, GFX_Viewport, 0);
  const GFX_Surface *surface = ecs_field(it, GFX_Surface, 1);

  for (int i = 0; i < it->count; ++i) {
    GFX_Viewport viewport = viewports[i];
    float w = viewport.rect[2] * surface->true_width;
    float h = viewport.rect[3] * surface->true_height;
    if (!viewport.camera || w <= 0.0f || h <= 0.0f) {
      continue;
    }
    const GFX_Camera *camera = ecs_get(it->world, viewport.camera, GFX_Camera);
    if (!camera) {
      continue;
    }

    mat4 view_projection;
    gfx_camera_view_projection(*camera, w / h, view_projection);
    if (!gfx_frame_view_begin(viewport.rect, view_projection)) {
      continue;
    }

    ecs_iter_t children = ecs_children(it->world, it->entities[i]);
    while (ecs_children_next(&children)) {
      for (int c = 0; c < children.count; ++c) {
        ecs_entity_t child = children.entities[c];
        const GFX_RenderBatch *batch =
            ecs_get(it->world, child, GFX_RenderBatch);
        if (batch && batch->draw) {
          batch->draw(it->world, child, &viewport);
        }
      }
    }
  }
}

void gfx_viewport_register(ecs_world_t *world) {
  ECS_COMPONENT_DEFINE(world, GFX_Viewport);
  ECS_COMPONENT_DEFINE(world, GFX_RenderBatch);

  // Renderers draw in the order they were added
  ecs_add_pair(world, ecs_id(GFX_Viewport), EcsWith, EcsOrderedChildren);

  ecs_system(
      world,
      {
          .entity = ecs_entity(
              world, {.name = "_gfx_sys_viewport_draw",
                      .add = ecs_ids(ecs_dependson(GfxRenderObjects))}),
          .query =
              {
                  .terms = {{ecs_id(GFX_Viewport)}, {ecs_id(GFX_Surface)}},
                  .order_by = ecs_id(GFX_Viewport),
                  .order_by_callback = _gfx_viewport_compare,
              },
          .callback = _gfx_sys_viewport_draw,
      });
}
