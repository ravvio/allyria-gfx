#include <gfx/gfx.h>

static ecs_entity_t camera_create(ecs_world_t *world, float size) {
  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {
              .projection = GLM_MAT4_IDENTITY_INIT,
              .view = GLM_MAT4_IDENTITY_INIT,
          });
  ecs_set(world, cam, GFX_CameraOrtho, {.size = size});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
  return cam;
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
          {.title = "Engine Post Processing", .width = 800, .height = 600});
  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);
  ECS_IMPORT(world, GfxPostEffect);

  // The scene is drawn into a render texture...
  GFX_RenderTexture screen =
      gfx_render_texture_create(800, 600, GFX_TEXTURE_FILTER_NEAREST);
  ecs_entity_t scene_cam = camera_create(world, 120.0f);
  ecs_entity_t scene_view = ecs_new(world);
  ecs_set(world, scene_view, GFX_Viewport,
          {.camera = scene_cam,
           .texture = &screen,
           .surface = window,
           .rect = {0, 0, 1, 1}});
  ecs_add(world, scene_view, GfxFitSurface);
  ecs_add(world, ecs_new_w_pair(world, EcsChildOf, scene_view),
          GFX_SpriteBatch);

  // ...then desaturated into the window, by a viewport without camera
  GFX_Shader shader_desaturate = gfx_shader_create(
      "./assets/shaders/post_processing_desaturate.wgsl", sizeof(float), 1);
  float strength = 0.5f;
  const GFX_Texture2D *textures[] = {&screen.texture};
  const GFX_PostEffect effect_desaturate =
      gfx_post_effect_create(&shader_desaturate, &strength, textures);

  ecs_entity_t post_view = ecs_new(world);
  ecs_set(world, post_view, GFX_Viewport,
          {.surface = window, .rect = {0, 0, 1, 1}});
  ecs_entity_t post_effect = ecs_new_w_pair(world, EcsChildOf, post_view);
  ecs_set_ptr(world, post_effect, GFX_PostEffect, &effect_desaturate);

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
    glm_translate_x(*t, 24.0f * (i - 4.5f));
  }

  gfx_run(world);

  gfx_post_effect_destroy(&effect_desaturate);
  gfx_shader_destroy(&shader_desaturate);
  gfx_texture2d_destroy(&atlas);
  gfx_render_texture_destroy(&screen);
  gfx_terminate(world);
  return 0;
}
