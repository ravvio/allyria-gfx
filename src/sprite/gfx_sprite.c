#include "../../include/gfx/sprite/gfx_sprite.h"
#include "../../include/gfx/gfx.h"
#include <stdio.h>

ECS_COMPONENT_DECLARE(GFX_Sprite);
ECS_COMPONENT_DECLARE(GFX_SpriteLayer);
ECS_COMPONENT_DECLARE(GFX_SpriteRenderer);

// *** SPRITE ***

GFX_Sprite gfx_sprite_create(GFX_Texture2D *texture_atlas, int offset_x,
                             int offset_y, int width, int height,
                             u_int8_t layer) {
  assert(layer < GFX_SPRITE_LAYERS_COUNT);

  // top left
  vec2 p0 = {
      (float)offset_x / texture_atlas->width,
      (float)offset_y / texture_atlas->height,
  };
  // bottom right
  vec2 p1 = {
      (float)(offset_x + width) / texture_atlas->width,
      (float)(offset_y + height) / texture_atlas->height,
  };

  // TODO: the size of the mesh should be scaled based on a pixel ratio
  GFX_Vertex2D vertices[4] = {
      gfx_vertex2d_create(-width / 2.0, +height / 2.0, p0[0], p0[1]),
      gfx_vertex2d_create(-width / 2.0, -height / 2.0, p0[0], p1[1]),
      gfx_vertex2d_create(+width / 2.0, -height / 2.0, p1[0], p1[1]),
      gfx_vertex2d_create(+width / 2.0, +height / 2.0, p1[0], p0[1]),
  };
  u_int32_t indices[6] = {0, 1, 2, 2, 3, 0};

  GFX_Geometry2D quad = gfx_geometry2d_create(4, vertices, 6, indices);

  GFX_Sprite sprite = {
      .quad = quad,
      .texture_atlas = texture_atlas,
      .model = GLM_MAT4_IDENTITY_INIT,
  };
  return sprite;
}

// *** SPRITE RENDERER ***

void gfx_sprite_renderer_add_sprite(GFX_SpriteRenderer *sr, GFX_Sprite *sprite,
                                    GFX_SpriteLayer layer) {
  arrput(sr->layers[layer], sprite);
}

void gfx_sprite_renderer_clear(GFX_SpriteRenderer *sr) {
  for (int i = 0; i < GFX_SPRITE_LAYERS_COUNT; i++) {
    arrsetlen(sr->layers[i], 0);
  }
}

void gfx_sprite_renderer_destroy(GFX_SpriteRenderer *sr) {
  for (int i = 0; i < GFX_SPRITE_LAYERS_COUNT; i++) {
    if (sr->layers[i]) {
      arrfree(sr->layers[i]);
    }
  }
}

ECS_DTOR(GFX_SpriteRenderer, ptr, { gfx_sprite_renderer_destroy(ptr); })

ECS_MOVE(GFX_SpriteRenderer, dst, src, {
  gfx_sprite_renderer_destroy(dst);
  *dst = *src;
  ecs_os_zeromem(src);
})

// *** SYSTEMS ***

// Sync sprite model and transform
void _gfx_sys_sprite_sync_transform(ecs_iter_t *it) {
  GFX_Sprite *sprites = ecs_field(it, GFX_Sprite, 0);
  GFX_Transform *transforms = ecs_field(it, GFX_Transform, 1);

  for (int i = 0; i < it->count; i++) {
    glm_mat4_copy(transforms[i], sprites[i].model);
  }
}

// Upload the sprites to the renderer
void _gfx_sys_sprite_upload(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_sprite_upload: start\n");
#endif
  // Load and clear the sprite renderer
  GFX_SpriteRenderer *sr = ecs_singleton_get_mut(it->world, GFX_SpriteRenderer);
  gfx_sprite_renderer_clear(sr);

  GFX_Sprite *sprites = ecs_field(it, GFX_Sprite, 0);
  GFX_SpriteLayer *layers = ecs_field(it, GFX_SpriteLayer, 1);

  // Upload sprites
#ifdef GFX_VERBOSE
  printf("[i] sys_sprite_upload: uploading %d sprites\n", it->count);
#endif

  for (int i = 0; i < it->count; i++) {
    gfx_sprite_renderer_add_sprite(sr, &sprites[i], layers[i]);
  }

#ifdef GFX_VERBOSE
  printf("[i] sys_sprite_upload: done\n");
#endif
}

void _gfx_sys_sprite_draw(ecs_iter_t *it) {
#ifdef GFX_VERBOSE
  printf("[i] sys_sprite_draw: start\n");
#endif
  GFX_SpriteRenderer const *sr =
      ecs_singleton_get(it->world, GFX_SpriteRenderer);
  GFX_Engine const *engine = ecs_singleton_get(it->world, GFX_Engine);

  assert(engine->uniforms_id != 0);
  GFX_Uniforms const *uniforms =
      ecs_get(it->world, engine->uniforms_id, GFX_Uniforms);

  gfx_shader_uniform_set_mat4(&sr->shader, "t_camera_mvp", false,
                              uniforms->t_camera_mvp);
  gfx_shader_bind(&sr->shader);

  for (int i = 0; i < GFX_SPRITE_LAYERS_COUNT; i++) {
#ifdef GFX_VERBOSE
    printf("[i] drawing sprite layer %d | count: %td\n", i,
           arrlen(sr->layers[i]));
#endif
    for (long j = 0; j < arrlen(sr->layers[i]); j++) {
      GFX_Sprite *sprite = sr->layers[i][j];
      gfx_shader_uniform_set_mat4(&sr->shader, "t_model", false, sprite->model);
      gfx_texture2d_bind(sprite->texture_atlas);
      gfx_geometry2d_draw(&sprite->quad);
    }
  }
}

// *** MODULE ***

void GfxSpriteImport(ecs_world_t *world) {
  ECS_MODULE(world, GfxSprite);
  ecs_set_name_prefix(world, "Gfx");

  ECS_COMPONENT_DEFINE(world, GFX_Sprite);
  ECS_COMPONENT_DEFINE(world, GFX_SpriteLayer);
  ECS_COMPONENT_DEFINE(world, GFX_SpriteRenderer);
  ecs_set_hooks(world, GFX_SpriteRenderer,
                {
                    .ctor = flecs_default_ctor,
                    .dtor = ecs_dtor(GFX_SpriteRenderer),
                    .move = ecs_move(GFX_SpriteRenderer),
                });

  // Sync systems
  ECS_SYSTEM(world, _gfx_sys_sprite_sync_transform, EcsOnUpdate, GFX_Sprite, GFX_Transform);

  // Rendering systems
  ECS_SYSTEM(world, _gfx_sys_sprite_upload, GfxStartRender, GFX_Sprite,
             GFX_SpriteLayer);
  ECS_SYSTEM(world, _gfx_sys_sprite_draw, GfxRenderObjects, 0);

  // Create the sprite renderer
  GFX_Shader shader = gfx_shader_create("./assets/shaders/sprite.vert",
                                        "./assets/shaders/sprite.frag");
  ecs_singleton_set(world, GFX_SpriteRenderer,
                    {
                        .shader = shader,
                    });
}
