#include <gfx/gfx.h>
#include <math.h>

ECS_TAG_DECLARE(Panning);

// Move panning cameras left and right over the sprites
static void pan(ecs_iter_t *it) {
  GFX_Transform *t = ecs_field(it, GFX_Transform, 0);
  float x =
      72.0f + 96.0f * sinf(ecs_get_world_info(it->world)->world_time_total);
  for (int i = 0; i < it->count; ++i) {
    glm_translate_make(t[i], (vec3){x, 0.0f, 0.0f});
  }
}

static ecs_entity_t camera_create(ecs_world_t *world, float size, float x) {
  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {
              .projection = GLM_MAT4_IDENTITY_INIT,
              .view = GLM_MAT4_IDENTITY_INIT,
          });
  ecs_set(world, cam, GFX_CameraOrtho, {.size = size});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
  GFX_Transform *t = ecs_get_mut(world, cam, GFX_Transform);
  glm_translate_x(*t, x);
  return cam;
}

// A viewport showing camera, with the sprites drawn into it
static void viewport_create(ecs_world_t *world, ecs_entity_t window,
                            ecs_entity_t camera, float x, float y, float w,
                            float h, int32_t order) {
  ecs_entity_t view = ecs_new(world);
  ecs_set(world, view, GFX_Viewport,
          {.camera = camera,
           .surface = window,
           .rect = {x, y, w, h},
           .order = order});
  ecs_add(world, ecs_new_w_pair(world, EcsChildOf, view), GFX_SpriteBatch);
}

int main(void) {
  enum GFX_GraphicsFeature graphics_features[] = {
      GFX_GRAPHICS_FEATURE_ALPHA_BLEND,
  };
  GFX_ProjectSettings settings = {
      .graphics_features = graphics_features,
      .graphics_features_count =
          sizeof(graphics_features) / sizeof(graphics_features[0]),
      .clear_color = {0.2, 0.3, 0.4, 1.0},
  };
  ecs_world_t *world = ecs_init();

  if (gfx_init(&settings, world) != GFX_ERR_OK) {
    gfx_terminate(world);
    return 1;
  }
  ecs_entity_t window = ecs_new(world);
  ecs_set(world, window, GFX_Surface,
          {.title = "Engine Viewports", .width = 800, .height = 600});

  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  ECS_TAG_DEFINE(world, Panning);
  ECS_SYSTEM(world, pan, EcsOnUpdate, GFX_Transform, Panning);

  // Split screen: a still camera on the left, a panning one on the right
  ecs_entity_t left = camera_create(world, 128.0f, 72.0f);
  ecs_entity_t right = camera_create(world, 128.0f, 72.0f);
  ecs_add(world, right, Panning);
  viewport_create(world, window, left, 0.0f, 0.0f, 0.5f, 1.0f, 0);
  viewport_create(world, window, right, 0.5f, 0.0f, 0.5f, 1.0f, 0);

  // Minimap on top of the right half, zoomed out
  ecs_entity_t minimap = camera_create(world, 32.0f, 72.0f);
  viewport_create(world, window, minimap, 0.72f, 0.04f, 0.24f, 0.24f, 1);

  GFX_Texture2D atlas =
      gfx_texture2d_load("./assets/textures/sprites/sensei.png",
                         GFX_TEXTURE_FILTER_NEAREST, GFX_WRAP_REPEAT);

  for (int i = 0; i < 10; ++i) {
    ecs_entity_t e = ecs_new(world);
    GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 23);
    ecs_set_ptr(world, e, GFX_Sprite, &sprite);
    ecs_set(world, e, GFX_SpriteLayer, {0});
    ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
    GFX_Transform *t = ecs_get_mut(world, e, GFX_Transform);
    glm_translate_x(*t, 16.0 * i);
  }

  gfx_run(world);

  gfx_texture2d_destroy(&atlas);
  gfx_terminate(world);
  return 0;
}
