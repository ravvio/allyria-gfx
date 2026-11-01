#include <gfx/engine.h>
#include <gfx/render.h>
#include <stdio.h>

ECS_TAG_DECLARE(GfxPreRender);
ECS_TAG_DECLARE(GfxStartRender);
ECS_TAG_DECLARE(GfxRenderObjects);
ECS_TAG_DECLARE(GfxRenderGizmos);
ECS_TAG_DECLARE(GfxRenderGUI);
ECS_TAG_DECLARE(GfxEndRender);
ECS_TAG_DECLARE(GfxPostRender);

// Sync the surface and ready it for the frame
void _gfx_sys_surface_sync(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_surface_sync: start | dt %f\n", it->delta_time);
#endif

  GFX_Surface *surface = ecs_field(it, GFX_Surface, 0);
  if (surface->window) {
    gfx_surface_sync(surface);
  }
}

// Start drawing the frame
void _gfx_sys_start_render(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_start_render: start\n");
#endif

  const GFX_Surface *surface = ecs_field(it, GFX_Surface, 0);
  if (surface->window) {
    gfx_frame_begin(surface->window);
  }

#ifdef GFX_VERBOSE
  printf("[i] sys_start_render: done\n");
#endif
}

// End render, submit and present
void _gfx_sys_end_render(ecs_iter_t *it) {
  const GFX_Surface *surface = ecs_field(it, GFX_Surface, 0);
  if (surface->window) {
    gfx_frame_end(surface->window);
  }
}

// Poll events
void _gfx_sys_post_render(ecs_iter_t *it) { gfx_window_events_poll(); }

void GfxRendererImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxRenderer);
  ecs_set_name_prefix(world, "Gfx");

  gfx_surface_register(world);

  // Rendering Phases
  GfxPreRender = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxPreRender, EcsDependsOn, EcsOnStore);

  GfxStartRender = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxStartRender, EcsDependsOn, GfxPreRender);

  GfxRenderObjects = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxRenderObjects, EcsDependsOn, GfxStartRender);

  GfxRenderGizmos = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxRenderGizmos, EcsDependsOn, GfxRenderObjects);

  GfxRenderGUI = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxRenderGUI, EcsDependsOn, GfxRenderGizmos);

  GfxEndRender = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxEndRender, EcsDependsOn, GfxRenderGUI);

  GfxPostRender = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, GfxPostRender, EcsDependsOn, GfxEndRender);

  // Systems

  ECS_SYSTEM(world, _gfx_sys_surface_sync, EcsOnLoad, GFX_Surface);
  ECS_SYSTEM(world, _gfx_sys_start_render, GfxStartRender, GFX_Surface);
  ECS_SYSTEM(world, _gfx_sys_end_render, GfxEndRender, GFX_Surface);
  ECS_SYSTEM(world, _gfx_sys_post_render, GfxPostRender, 0);

  gfx_viewport_register(world);
}
