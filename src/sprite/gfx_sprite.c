// Instanced sprites: every frame the sprites are collected, sorted by (layer,
// texture) and uploaded as one instance buffer, then drawn with one draw per
// run of sprites sharing a texture.

#include <gfx/engine.h>
#include <gfx/sprite.h>
#include <gfx/transform.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ECS_COMPONENT_DECLARE(GFX_Sprite);
ECS_COMPONENT_DECLARE(GFX_SpriteLayer);
ECS_COMPONENT_DECLARE(GFX_SpriteRenderer);

// A sprite to draw this frame, with its sort keys
struct _GFX_SpriteDraw {
  GFX_SpriteLayer layer;
  const GFX_Texture2D *texture;
  GFX_SpriteInstance instance;
};

struct _GFX_SpriteMaterial {
  WGPUTextureView key;
  GFX_Material value;
};

#define GFX_SPRITE_INITIAL_CAPACITY 64

static const char _gfx_sprite_wgsl[] = {
#embed "sprite.wgsl"
    , 0};

// *** SPRITE ***

GFX_Sprite gfx_sprite_create(const GFX_Texture2D *texture, int offset_x,
                             int offset_y, int width, int height) {
  GFX_Sprite sprite = {
      .texture = texture,
      .uv_rect = {0.0, 0.0, 1.0, 1.0},
      .size = {width, height},
  };
  // Failed loads have no size, draw the whole (white) texture
  if (texture->width > 0 && texture->height > 0) {
    sprite.uv_rect[0] = (float)offset_x / texture->width;
    sprite.uv_rect[1] = (float)offset_y / texture->height;
    sprite.uv_rect[2] = (float)width / texture->width;
    sprite.uv_rect[3] = (float)height / texture->height;
  }
  return sprite;
}

// *** SPRITE RENDERER ***

static int _gfx_sprite_draw_compare(const void *a, const void *b) {
  const struct _GFX_SpriteDraw *da = a;
  const struct _GFX_SpriteDraw *db = b;
  if (da->layer != db->layer) {
    return da->layer < db->layer ? -1 : 1;
  }
  if (da->texture->view != db->texture->view) {
    return (uintptr_t)da->texture->view < (uintptr_t)db->texture->view ? -1 : 1;
  }
  return 0;
}

// Material of a texture, created on first use
static const GFX_Material *_gfx_sprite_material(GFX_SpriteRenderer *sr,
                                                const GFX_Texture2D *texture) {
  ptrdiff_t i = hmgeti(sr->materials, texture->view);
  if (i < 0) {
    const GFX_Texture2D *textures[] = {texture};
    GFX_Material material =
        gfx_material_create(&sr->material_layout, NULL, textures);
    hmput(sr->materials, texture->view, material);
    i = hmgeti(sr->materials, texture->view);
  }
  return &sr->materials[i].value;
}

static void _gfx_sprite_renderer_destroy(GFX_SpriteRenderer *sr) {
  for (ptrdiff_t i = 0; i < hmlen(sr->materials); i++) {
    gfx_material_destroy(&sr->materials[i].value);
  }
  hmfree(sr->materials);
  arrfree(sr->draws);
  arrfree(sr->instances);
  if (sr->instance_buffer.buffer) {
    gfx_vertex_buffer_destroy(&sr->instance_buffer);
  }
  if (sr->pipeline.pipeline) {
    gfx_pipeline_destroy(&sr->pipeline);
    gfx_material_layout_destroy(&sr->material_layout);
    gfx_shader_destroy(&sr->shader);
  }
  // The query belongs to the world
  memset(sr, 0, sizeof(*sr));
}

ECS_DTOR(GFX_SpriteRenderer, ptr, { _gfx_sprite_renderer_destroy(ptr); })

ECS_MOVE(GFX_SpriteRenderer, dst, src, {
  _gfx_sprite_renderer_destroy(dst);
  *dst = *src;
  ecs_os_zeromem(src);
})

// *** SYSTEMS ***

// Collect, sort, upload and draw every sprite
void _gfx_sys_sprite_draw(ecs_iter_t *it) {
  GFX_SpriteRenderer *sr = ecs_singleton_get_mut(it->world, GFX_SpriteRenderer);

  // Collect the sprites of every table
  arrsetlen(sr->draws, 0);
  ecs_iter_t q = ecs_query_iter(it->world, sr->query);
  while (ecs_query_next(&q)) {
    GFX_Sprite *sprites = ecs_field(&q, GFX_Sprite, 0);
    GFX_Transform *transforms = ecs_field(&q, GFX_Transform, 1);
    GFX_SpriteLayer *layers = ecs_field(&q, GFX_SpriteLayer, 2);

    for (int i = 0; i < q.count; i++) {
      struct _GFX_SpriteDraw draw = {
          .layer = layers[i],
          .texture = sprites[i].texture,
      };
      // Scale the unit quad to the sprite size in pixels
      vec3 size = {sprites[i].size[0], sprites[i].size[1], 1.0};
      glm_mat4_copy(transforms[i], draw.instance.model);
      glm_scale(draw.instance.model, size);
      glm_vec4_copy(sprites[i].uv_rect, draw.instance.uv_rect);
      arrput(sr->draws, draw);
    }
  }

  uint32_t count = arrlen(sr->draws);
  if (count == 0) {
    return;
  }
  qsort(sr->draws, count, sizeof(struct _GFX_SpriteDraw),
        _gfx_sprite_draw_compare);

  // Upload the instances in draw order
  arrsetlen(sr->instances, count);
  for (uint32_t i = 0; i < count; i++) {
    sr->instances[i] = sr->draws[i].instance;
  }
  if (count > sr->instance_capacity) {
    gfx_vertex_buffer_destroy(&sr->instance_buffer);
    while (sr->instance_capacity < count) {
      sr->instance_capacity *= 2;
    }
    sr->instance_buffer = gfx_vertex_buffer_create(
        sr->instance_capacity * sizeof(GFX_SpriteInstance), NULL);
  }
  gfx_vertex_buffer_write(&sr->instance_buffer, sr->instances,
                          count * sizeof(GFX_SpriteInstance));

  // One draw per run of sprites sharing a texture, instances keep the order
  gfx_pipeline_bind(&sr->pipeline);
  gfx_vertex_buffer_bind(&sr->instance_buffer);
  uint32_t start = 0;
  while (start < count) {
    uint32_t end = start + 1;
    while (end < count &&
           sr->draws[end].texture->view == sr->draws[start].texture->view) {
      end++;
    }
    gfx_material_bind(_gfx_sprite_material(sr, sr->draws[start].texture));
    // 6 vertices, the quad corners come from the vertex index
    gfx_draw_instanced(6, end - start, start);
    start = end;
  }

#ifdef GFX_VERBOSE
  printf("[i] sys_sprite_draw: %u sprites\n", count);
#endif
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

  ECS_SYSTEM(world, _gfx_sys_sprite_draw, GfxRenderObjects, 0);

  // Per instance: model matrix columns (locations 0-3), uv rect (4)
  GFX_VertexLayout instance_layout = {.step = WGPUVertexStepMode_Instance};
  for (int i = 0; i < 5; i++) {
    gfx_vertex_layout_push_f32(&instance_layout, 4);
  }

  GFX_SpriteRenderer sr = {
      .shader = gfx_shader_create_wgsl("sprite.wgsl", _gfx_sprite_wgsl),
      // No uniform, one texture
      .material_layout = gfx_material_layout_create(0, 1),
      .query = ecs_query(world, {.terms = {{ecs_id(GFX_Sprite)},
                                           {ecs_id(GFX_Transform)},
                                           {ecs_id(GFX_SpriteLayer)}}}),
      .instance_capacity = GFX_SPRITE_INITIAL_CAPACITY,
  };
  sr.pipeline =
      gfx_pipeline_create(&sr.shader, &instance_layout, &sr.material_layout);
  sr.instance_buffer = gfx_vertex_buffer_create(
      sr.instance_capacity * sizeof(GFX_SpriteInstance), NULL);
  ecs_singleton_set_ptr(world, GFX_SpriteRenderer, &sr);
}
