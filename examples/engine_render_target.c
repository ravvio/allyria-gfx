#include <gfx/gfx.h>
#include <math.h>

// The sprites of the scene, drawn into the render texture
ECS_TAG_DECLARE(Scene);
// The render texture shown in the window
ECS_TAG_DECLARE(Screen);

// Move the scene camera left and right
static void pan(ecs_iter_t *it) {
  GFX_Transform *t = ecs_field(it, GFX_Transform, 0);
  float x =
      72.0f + 48.0f * sinf(ecs_get_world_info(it->world)->world_time_total);
  for (int i = 0; i < it->count; ++i) {
    glm_translate_make(t[i], (vec3){x, 0.0f, 0.0f});
  }
}

static ecs_entity_t camera_create(ecs_world_t *world, float ratio) {
  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {
              .projection = GLM_MAT4_IDENTITY_INIT,
              .view = GLM_MAT4_IDENTITY_INIT,
              .model = GLM_MAT4_IDENTITY_INIT,
          });
  ecs_set(world, cam, GFX_CameraOrtho, {.size = 1.0});
  ecs_set(world, cam, GFX_CameraPixel, {.ratio = ratio});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
  return cam;
}

static void sprite_create(ecs_world_t *world, GFX_Sprite sprite, float x,
                          ecs_entity_t tag) {
  ecs_entity_t e = ecs_new(world);
  ecs_set_ptr(world, e, GFX_Sprite, &sprite);
  ecs_set(world, e, GFX_SpriteLayer, {0});
  ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
  GFX_Transform *t = ecs_get_mut(world, e, GFX_Transform);
  glm_translate_x(*t, x);
  ecs_add_id(world, e, tag);
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
  ecs_singleton_set(
      world, GFX_Surface,
      {.title = "Engine Render Target", .width = 800, .height = 600});

  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  ECS_TAG_DEFINE(world, Scene);
  ECS_TAG_DEFINE(world, Screen);
  ECS_TAG(world, Panning);
  ECS_SYSTEM(world, pan, EcsOnUpdate, GFX_Transform, Panning);

  // The scene at 160x120 pixels, one texel per sprite pixel
  GFX_RenderTexture screen =
      gfx_render_texture_create(160, 120, GFX_TEXTURE_FILTER_NEAREST);
  ecs_entity_t scene_cam = camera_create(world, 60.0f);
  ecs_add(world, scene_cam, Panning);
  ecs_entity_t scene_view = ecs_new(world);
  ecs_set(world, scene_view, GFX_Viewport,
          {.camera = scene_cam, .rect = {0, 0, 1, 1}, .target = &screen});
  ecs_set(world, ecs_new_w_pair(world, EcsChildOf, scene_view), GFX_SpriteBatch,
          {.tag = Scene});

  // The window shows the render texture as one sprite filling its height
  ecs_entity_t window_cam = camera_create(world, 60.0f);
  ecs_entity_t window_view = ecs_new(world);
  ecs_set(world, window_view, GFX_Viewport,
          {.camera = window_cam, .rect = {0, 0, 1, 1}});
  ecs_set(world, ecs_new_w_pair(world, EcsChildOf, window_view),
          GFX_SpriteBatch, {.tag = Screen});
  sprite_create(world, gfx_sprite_create(&screen.texture, 0, 0, 160, 120), 0,
                Screen);

  GFX_Texture2D atlas =
      gfx_texture2d_load("./assets/textures/sprites/sensei.png",
                         GFX_TEXTURE_FILTER_NEAREST, GFX_WRAP_REPEAT);
  for (int i = 0; i < 10; ++i) {
    sprite_create(world, gfx_sprite_create(&atlas, 0, 0, 16, 23), 16.0f * i,
                  Scene);
  }

  gfx_run(world);

  gfx_texture2d_destroy(&atlas);
  gfx_render_texture_destroy(&screen);
  gfx_terminate(world);
  return 0;
}
