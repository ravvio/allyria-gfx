#include "../../include/gfx/gfx.h"
#include <stdio.h>

ECS_COMPONENT_DECLARE(GFX_Uniforms);

ECS_TAG_DECLARE(GfxPreRender);
ECS_TAG_DECLARE(GfxStartRender);
ECS_TAG_DECLARE(GfxRenderObjects);
ECS_TAG_DECLARE(GfxRenderGizmos);
ECS_TAG_DECLARE(GfxRenderGUI);
ECS_TAG_DECLARE(GfxEndRender);
ECS_TAG_DECLARE(GfxPostRender);

void _gfx_sys_pre_render(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_pre_render: start | dt %f\n", it->delta_time);
#endif

  // Load and prepare surface
  const GFX_Engine *engine = ecs_singleton_get(it->world, GFX_Engine);
  GFX_Surface *surface =
      ecs_get_mut(it->world, engine->surface_id, GFX_Surface);
  gfx_surface_sync(surface);

  GFX_Uniforms *uniforms = ecs_get_mut(it->world, engine->uniforms_id, GFX_Uniforms);

  // Sync camera values
  if (engine->main_camera_id > 0) {
    GFX_Camera *cam = ecs_get_mut(it->world, engine->main_camera_id, GFX_Camera);
    glm_mat4_mul(cam->projection, cam->view, uniforms->t_camera_mvp);
    glm_mat4_mul(uniforms->t_camera_mvp, cam->model, uniforms->t_camera_mvp);
  }

#ifdef GFX_VERBOSE
  printf("[i] sys_pre_render: done\n");
#endif
}

// Clear buffers and start drawing
void _gfx_sys_start_render(ecs_iter_t *it) { gfx_buffers_clear(); }

// End render, swap buffers
void _gfx_sys_end_render(ecs_iter_t *it) {
  const GFX_Engine *engine = ecs_singleton_get(it->world, GFX_Engine);
  GFX_Surface *surface =
      ecs_get_mut(it->world, engine->surface_id, GFX_Surface);
  gfx_window_swap_buffers(surface->window);
}

// Poll events
void _gfx_sys_post_render(ecs_iter_t *it) { gfx_window_events_poll(); }

void GfxRendererImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxRenderer);
  ecs_set_name_prefix(world, "Gfx");

  // Components

  ECS_COMPONENT_DEFINE(world, GFX_Uniforms);

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

  ECS_SYSTEM(world, _gfx_sys_pre_render, GfxPreRender, 0);
  ECS_SYSTEM(world, _gfx_sys_start_render, GfxStartRender, 0);
  ECS_SYSTEM(world, _gfx_sys_end_render, GfxEndRender, 0);
  ECS_SYSTEM(world, _gfx_sys_post_render, GfxPostRender, 0);
}
