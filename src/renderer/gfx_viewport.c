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
  // Render textures first, so the window can show them in the same frame
  if (!a->texture != !b->texture) {
    return a->texture ? -1 : 1;
  }
  return (a->order > b->order) - (a->order < b->order);
}

// Draw each viewport with its renderers, in order
void _gfx_sys_viewport_draw(ecs_iter_t *it) {
  GFX_Viewport *viewports = ecs_field(it, GFX_Viewport, 0);

  for (int i = 0; i < it->count; ++i) {
    GFX_Viewport viewport = viewports[i];
    if (!viewport.camera || !ecs_is_alive(it->world, viewport.camera)) {
      continue;
    }
    const GFX_Camera *camera = ecs_get(it->world, viewport.camera, GFX_Camera);
    if (!camera) {
      continue;
    }

    // Size of what it draws into
    GFX_RenderTexture *texture = viewport.texture;
    GFX_Window *window = NULL;
    float target_w, target_h;
    if (texture) {
      target_w = texture->texture.width;
      target_h = texture->texture.height;
    } else {
      if (!viewport.surface || !ecs_is_alive(it->world, viewport.surface)) {
        continue;
      }
      const GFX_Surface *surface =
          ecs_get(it->world, viewport.surface, GFX_Surface);
      if (!surface || !surface->window) {
        continue;
      }
      window = surface->window;
      target_w = surface->true_width;
      target_h = surface->true_height;
    }
    float w = viewport.rect[2] * target_w;
    float h = viewport.rect[3] * target_h;
    if (w <= 0.0f || h <= 0.0f) {
      continue;
    }

    mat4 view_projection;
    gfx_camera_view_projection(*camera, w / h, view_projection);
    bool begun =
        texture ? gfx_frame_view_begin_texture(texture, viewport.rect,
                                               view_projection)
                : gfx_frame_view_begin(window, viewport.rect, view_projection);
    if (!begun) {
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

  ecs_system(world,
             {
                 .entity = ecs_entity(
                     world, {.name = "_gfx_sys_viewport_draw",
                             .add = ecs_ids(ecs_dependson(GfxRenderObjects))}),
                 .query =
                     {
                         .terms = {{ecs_id(GFX_Viewport)}},
                         .order_by = ecs_id(GFX_Viewport),
                         .order_by_callback = _gfx_viewport_compare,
                     },
                 .callback = _gfx_sys_viewport_draw,
             });
}
