#include <gfx/engine.h>
#include <gfx/post.h>
#include <stdio.h>

ECS_COMPONENT_DECLARE(GFX_PostEffect);

static void _gfx_post_effect_draw(ecs_world_t *world, ecs_entity_t entity,
                                  const GFX_Viewport *viewport) {
  (void)viewport;
  GFX_PostEffect *effect = ecs_get_mut(world, entity, GFX_PostEffect);
  if (!effect->pipeline.pipeline) {
    return;
  }
  gfx_pipeline_bind(&effect->pipeline);
  gfx_material_bind(&effect->material);
  // The fullscreen triangle
  gfx_draw_instanced(3, 1, 0);

#ifdef GFX_VERBOSE
  printf("[i] post effect\n");
#endif
}

// Make the post effect a renderer of its viewport
static void _gfx_post_effect_on_add(ecs_iter_t *it) {
  for (int i = 0; i < it->count; i++) {
    ecs_set(it->world, it->entities[i], GFX_RenderBatch,
            {.draw = _gfx_post_effect_draw});
  }
}

// Material and fullscreen pipeline of shader, create it after the window
GFX_PostEffect gfx_post_effect_create(const GFX_Shader *shader,
                                      const void *uniform_data,
                                      const GFX_Texture2D *const *textures) {
  GFX_PostEffect effect = {
      .material = gfx_material_create(shader, uniform_data, textures),
      .pipeline = gfx_pipeline_fullscreen_create(shader),
  };
  return effect;
}

void gfx_post_effect_destroy(const GFX_PostEffect *effect) {
  gfx_pipeline_destroy(&effect->pipeline);
  gfx_material_destroy(&effect->material);
}

// *** MODULE ***

void GfxPostEffectImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxPostEffect);

  ECS_COMPONENT_DEFINE(world, GFX_PostEffect);
  ecs_set_hooks(world, GFX_PostEffect,
                {
                    .ctor = flecs_default_ctor,
                    .on_add = _gfx_post_effect_on_add,
                });
}
