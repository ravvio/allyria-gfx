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

  GFX_Surface *surfaces = ecs_field(it, GFX_Surface, 0);
  for (int i = 0; i < it->count; ++i) {
    if (surfaces[i].window) {
      gfx_surface_sync(&surfaces[i]);
    }
  }
}

// Start the frame and every window in it
void _gfx_sys_start_render(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_start_render: start\n");
#endif

  gfx_frame_begin();
  ecs_iter_t s = ecs_each(it->world, GFX_Surface);
  while (ecs_each_next(&s)) {
    GFX_Surface *surfaces = ecs_field(&s, GFX_Surface, 0);
    for (int i = 0; i < s.count; ++i) {
      if (surfaces[i].window) {
        gfx_frame_window_begin(surfaces[i].window);
      }
    }
  }
}

// End render, submit and present
void _gfx_sys_end_render(ecs_iter_t *it) {
  (void)it;
  gfx_frame_end();
}

// Poll events
void _gfx_sys_post_render(ecs_iter_t *it) { gfx_window_events_poll(); }

void GfxRendererImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxRenderer);

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
  ECS_SYSTEM(world, _gfx_sys_start_render, GfxStartRender, 0);
  ECS_SYSTEM(world, _gfx_sys_end_render, GfxEndRender, 0);
  ECS_SYSTEM(world, _gfx_sys_post_render, GfxPostRender, 0);

  gfx_viewport_register(world);
}
