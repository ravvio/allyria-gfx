#include <gfx/engine.h>
#include <stdio.h>

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

// Setting the surface again keeps its window
ECS_COPY(GFX_Surface, dst, src, {
  GFX_Window *window = dst->window;
  *dst = *src;
  dst->window = window;
})

// Open the window of new surfaces
static void _gfx_surface_on_set(ecs_iter_t *it) {
  GFX_Surface *surfaces = ecs_field(it, GFX_Surface, 0);
  for (int i = 0; i < it->count; ++i) {
    GFX_Surface *surface = &surfaces[i];
    if (surface->window) {
      continue;
    }
    surface->window =
        gfx_window_init(surface->width, surface->height, surface->title);
    if (!surface->window) {
      fprintf(stderr, "[e] failed to open the window\n");
      continue;
    }
    gfx_surface_sync(surface);
  }
}

void gfx_surface_register(ecs_world_t *world) {
  ECS_COMPONENT_DEFINE(world, GFX_Surface);
  ecs_set_hooks(world, GFX_Surface,
                {
                    .ctor = flecs_default_ctor,
                    .dtor = ecs_dtor(GFX_Surface),
                    .move = ecs_move(GFX_Surface),
                    .copy = ecs_copy(GFX_Surface),
                    .on_set = _gfx_surface_on_set,
                });
  ecs_add_id(world, ecs_id(GFX_Surface), EcsSingleton);
}
