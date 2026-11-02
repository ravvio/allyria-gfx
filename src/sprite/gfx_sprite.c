// Instanced sprites: sorted by layer and texture, drawn with one draw per run
// of sprites sharing a texture

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
ECS_COMPONENT_DECLARE(GFX_SpriteBatch);

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

// Material of a texture, created on first use
static const GFX_Material *_gfx_sprite_material(GFX_SpriteRenderer *sr,
                                                const GFX_Texture2D *texture) {
  ptrdiff_t i = hmgeti(sr->materials, texture->view);
  if (i < 0) {
    const GFX_Texture2D *textures[] = {texture};
    GFX_Material material = gfx_material_create(sr->shader, NULL, textures);
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
  if (sr->pipeline.pipeline) {
    gfx_pipeline_destroy(&sr->pipeline);
  }
  if (sr->shader) {
    gfx_shader_destroy(sr->shader);
    free(sr->shader);
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

// *** SPRITE BATCH ***

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

// Collect, sort, upload and draw every sprite
static void _gfx_sprite_batch_draw(ecs_world_t *world, ecs_entity_t entity,
                                   const GFX_Viewport *viewport) {
  (void)viewport;
  GFX_SpriteRenderer *sr = ecs_singleton_get_mut(world, GFX_SpriteRenderer);
  GFX_SpriteBatch *batch = ecs_get_mut(world, entity, GFX_SpriteBatch);

  // Collect the sprites of every table
  arrsetlen(batch->draws, 0);
  ecs_iter_t q = ecs_query_iter(world, sr->query);
  while (ecs_query_next(&q)) {
    if (batch->tag &&
        !ecs_table_has_id(ecs_get_world(world), q.table, batch->tag)) {
      continue;
    }
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
      arrput(batch->draws, draw);
    }
  }

  uint32_t count = arrlen(batch->draws);
  if (count == 0) {
    return;
  }
  qsort(batch->draws, count, sizeof(struct _GFX_SpriteDraw),
        _gfx_sprite_draw_compare);

  // Upload the instances in draw order
  arrsetlen(batch->instances, count);
  for (uint32_t i = 0; i < count; i++) {
    batch->instances[i] = batch->draws[i].instance;
  }
  if (count > batch->instance_capacity) {
    if (batch->instance_buffer.buffer) {
      gfx_vertex_buffer_destroy(&batch->instance_buffer);
    }
    if (batch->instance_capacity == 0) {
      batch->instance_capacity = GFX_SPRITE_INITIAL_CAPACITY;
    }
    while (batch->instance_capacity < count) {
      batch->instance_capacity *= 2;
    }
    batch->instance_buffer = gfx_vertex_buffer_create(
        batch->instance_capacity * sizeof(GFX_SpriteInstance), NULL);
  }
  gfx_vertex_buffer_write(&batch->instance_buffer, batch->instances,
                          count * sizeof(GFX_SpriteInstance));

  // Needs the window, so it's created on the first draw
  if (!sr->pipeline.pipeline) {
    // Per instance: model matrix columns (locations 0-3), uv rect (4)
    GFX_VertexLayout layout = {.step = WGPUVertexStepMode_Instance};
    for (int i = 0; i < 5; i++) {
      gfx_vertex_layout_push_f32(&layout, 4);
    }
    sr->pipeline = gfx_pipeline_create(sr->shader, &layout);
  }

  // One draw per run of sprites sharing a texture, instances keep the order
  gfx_pipeline_bind(&sr->pipeline);
  gfx_vertex_buffer_bind(&batch->instance_buffer);
  uint32_t start = 0;
  while (start < count) {
    uint32_t end = start + 1;
    while (end < count && batch->draws[end].texture->view ==
                              batch->draws[start].texture->view) {
      end++;
    }
    gfx_material_bind(_gfx_sprite_material(sr, batch->draws[start].texture));
    // 6 vertices, the quad corners come from the vertex index
    gfx_draw_instanced(6, end - start, start);
    start = end;
  }

#ifdef GFX_VERBOSE
  printf("[i] sprite batch: %u sprites\n", count);
#endif
}

static void _gfx_sprite_batch_destroy(GFX_SpriteBatch *batch) {
  arrfree(batch->draws);
  arrfree(batch->instances);
  if (batch->instance_buffer.buffer) {
    gfx_vertex_buffer_destroy(&batch->instance_buffer);
  }
  memset(batch, 0, sizeof(*batch));
}

ECS_DTOR(GFX_SpriteBatch, ptr, { _gfx_sprite_batch_destroy(ptr); })

ECS_MOVE(GFX_SpriteBatch, dst, src, {
  _gfx_sprite_batch_destroy(dst);
  *dst = *src;
  ecs_os_zeromem(src);
})

// Make the batch a renderer of its viewport
static void _gfx_sprite_batch_on_add(ecs_iter_t *it) {
  for (int i = 0; i < it->count; i++) {
    ecs_set(it->world, it->entities[i], GFX_RenderBatch,
            {.draw = _gfx_sprite_batch_draw});
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
  ECS_COMPONENT_DEFINE(world, GFX_SpriteBatch);
  ecs_set_hooks(world, GFX_SpriteBatch,
                {
                    .ctor = flecs_default_ctor,
                    .dtor = ecs_dtor(GFX_SpriteBatch),
                    .move = ecs_move(GFX_SpriteBatch),
                    .on_add = _gfx_sprite_batch_on_add,
                });

  GFX_SpriteRenderer sr = {
      .shader = malloc(sizeof(GFX_Shader)),
      .query = ecs_query(world, {.terms = {{ecs_id(GFX_Sprite)},
                                           {ecs_id(GFX_Transform)},
                                           {ecs_id(GFX_SpriteLayer)}}}),
  };
  // No uniform, one texture
  *sr.shader = gfx_shader_create_wgsl("sprite.wgsl", _gfx_sprite_wgsl, 0, 1);
  ecs_singleton_set_ptr(world, GFX_SpriteRenderer, &sr);
}
