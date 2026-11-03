#ifndef GFX_POST_DECL
#define GFX_POST_DECL

#include <flecs.h>
#include <gfx/core.h>
#include <gfx/material.h>
#include <gfx/render.h>

// A fullscreen effect drawn into the viewport it is a child of. Its shader
// only has frag_main, which reads @location(0) uv. The game destroys it.
typedef struct {
  GFX_Material material;
  GFX_Pipeline pipeline;
} GFX_PostEffect;
extern ECS_COMPONENT_DECLARE(GFX_PostEffect);

// Material and fullscreen pipeline of shader, create it after the window
GFX_PostEffect gfx_post_effect_create(const GFX_Shader *shader,
                                      const void *uniform_data,
                                      const GFX_Texture2D *const *textures);
void gfx_post_effect_destroy(const GFX_PostEffect *effect);

void GfxPostEffectImport(ecs_world_t *world);

#endif
