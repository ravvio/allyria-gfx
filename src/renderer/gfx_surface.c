#include "../../include/gfx/gfx.h"

ECS_COMPONENT_DECLARE(GFX_Surface);

// Sync surface parameters with window
void gfx_surface_sync(GFX_Surface *surface) {
  int w, h;
  gfx_window_size(surface->window, &w, &h);
  surface->width = w;
  surface->height = h;

  float sx, sy;
  gfx_window_scale(surface->window, &sx, &sy);
  surface->scale_x = sx;
  surface->scale_y = sy;

  int tw = w / sx;
  if (tw < 1)
    tw = 1;
  surface->true_width = tw;

  int th = h / sy;
  if (th < 1)
    th = 1;
  surface->true_height = th;
}

void gfx_surface_destroy(GFX_Surface *surface) {
  if (surface->window) {
    gfx_window_destroy(surface->window);
    surface->window = NULL;
  }
}

ECS_DTOR(GFX_Surface, ptr, { gfx_surface_destroy(ptr); })

ECS_MOVE(GFX_Surface, dst, src, {
  gfx_surface_destroy(dst);
  *dst = *src;
  ecs_os_zeromem(src);
})

void gfx_surface_register(ecs_world_t *world) {
  ECS_COMPONENT_DEFINE(world, GFX_Surface);
  ecs_set_hooks(world, GFX_Surface,
                {
                    .ctor = flecs_default_ctor,
                    .dtor = ecs_dtor(GFX_Surface),
                    .move = ecs_move(GFX_Surface),
                });
}
