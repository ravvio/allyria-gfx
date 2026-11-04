# Architecture

How the engine is put together, and the rules that keep it working. Build
instructions are in `README.md`, contributor conventions in `AGENTS.md`, the
references behind the design in `SOURCES.md`.

## Layers

```
game                 examples/*.c, links `gfx`, includes <gfx/gfx.h>
  │
ECS modules          GfxEngine, GfxRenderer (windows, phases, viewports),
  │                  GfxTransform, GfxCamera, GfxSprite, GfxPostEffect
  │                  use only the low-level API, never wgpu* directly
low-level API        src/graphics/: device and limits, windows, frames and
  │                  views, buffers, shaders, pipelines, uniforms,
  │                  materials, textures, geometry, compute
WebGPU + GLFW        Dawn natively, emdawnwebgpu in the browser
```

WebGPU state (device, queue, the frame's encoder and pass) lives in the `_gfx`
struct, `src/gfx_internal.h`.

## A minimal game

```c
#include <gfx/gfx.h>

int main(void) {
  GFX_ProjectSettings settings = {.clear_color = {0.2, 0.3, 0.4, 1.0}};
  ecs_world_t *world = ecs_init();
  if (gfx_init(&settings, world) != GFX_ERR_OK) {
    gfx_terminate(world);
    return 1;
  }
  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  // A window
  ecs_entity_t window = ecs_new(world);
  ecs_set(world, window, GFX_Surface,
          {.title = "Game", .width = 800, .height = 600});

  // A camera showing 128 world units above and below its center
  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {.projection = GLM_MAT4_IDENTITY_INIT, .view = GLM_MAT4_IDENTITY_INIT});
  ecs_set(world, cam, GFX_CameraOrtho, {.size = 128});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  // The camera in the whole window, with a sprite renderer
  ecs_entity_t view = ecs_new(world);
  ecs_set(world, view, GFX_Viewport,
          {.camera = cam, .surface = window, .rect = {0, 0, 1, 1}});
  ecs_add(world, ecs_new_w_pair(world, EcsChildOf, view), GFX_SpriteBatch);

  // A sprite
  GFX_Texture2D atlas = gfx_texture2d_load("./assets/sprites.png",
                                           GFX_TEXTURE_FILTER_NEAREST,
                                           GFX_WRAP_REPEAT);
  ecs_entity_t e = ecs_new(world);
  GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 16);
  ecs_set_ptr(world, e, GFX_Sprite, &sprite);
  ecs_set(world, e, GFX_SpriteLayer, {0});
  ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  gfx_run(world); // returns when every window is closed (never in the browser)

  gfx_texture2d_destroy(&atlas);
  gfx_terminate(world);
  return 0;
}
```

## Frame

`gfx_run` calls `ecs_progress` every frame. The phases, in order:

| Phase | What runs |
|---|---|
| `EcsOnLoad` | each `GFX_Surface` reads its window's size |
| `EcsPreUpdate` | closed windows are deleted; quit when none is left |
| `EcsOnUpdate`, `EcsPostUpdate` | gameplay |
| `GfxPreRender` | cameras: `view` = inverse of `GFX_Transform`, square `projection` |
| `GfxStartRender` | `gfx_frame_begin`, then every window is acquired and cleared |
| `GfxRenderObjects` | the viewport loop (below) |
| `GfxRenderGizmos`, `GfxRenderGUI` | empty for now |
| `GfxEndRender` | `gfx_frame_end`: upload matrices, one submit, present every window |
| `GfxPostRender` | poll window events |

Systems that depend on each other go in different phases: flecs doesn't order
systems inside a phase.

## Windows

A window is an entity with `GFX_Surface`. Setting the component opens the
window (`src/renderer/gfx_surface.c`); setting it again keeps the window.

- `width`/`height` are the window size in screen coordinates,
  `true_width`/`true_height` the same divided by the content scale. Both are
  updated every frame; viewports only use their ratio.
- Every window uses the first window's pixel format, so one pipeline draws in
  all of them.
- Closing a window deletes its entity. The app quits when no window is left.
- The browser has one window.

## Viewports and render batches

A viewport is an entity with `GFX_Viewport`: a camera (0 for none, then the
view-projection is the identity), where it draws (a window's `surface`, or a
render `texture`; with both, it draws into the texture and the surface only
gives the size for `GfxFitSurface`), a `rect` in fractions of the target from
the top left, and an `order`. Its children with
`GFX_RenderBatch` are its renderers, drawn in the order they were added.

The viewport loop (`src/renderer/gfx_viewport.c`), once per frame:

1. viewports drawing into textures first, then by `order` (higher on top);
2. view-projection = aspect fit × camera projection × camera view;
3. `gfx_frame_view_begin(window, rect, vp)` or
   `gfx_frame_view_begin_texture(texture, rect, vp)`;
4. `draw` of each render batch.

Draw code belongs in a render batch, not in a system: systems run once per
frame, batches once per viewport. A batch of your own:

```c
typedef struct {
  GFX_Geometry3D geometry;
  GFX_Material material;
  GFX_Pipeline pipeline;
  mat4 model;
} Cube;
ECS_COMPONENT_DECLARE(Cube);

static void cube_draw(ecs_world_t *world, ecs_entity_t batch,
                      const GFX_Viewport *viewport) {
  (void)viewport;
  Cube *cube = ecs_get_mut(world, batch, Cube);
  gfx_pipeline_bind(&cube->pipeline);
  gfx_material_bind(&cube->material);
  gfx_draw_set_model(cube->model);
  gfx_geometry3d_draw(&cube->geometry);
}

// After the window opened (the pipeline needs its format)
ecs_entity_t batch = ecs_new_w_pair(world, EcsChildOf, view);
ecs_set_ptr(world, batch, Cube, &cube);
ecs_set(world, batch, GFX_RenderBatch, {.draw = cube_draw});
```

Every view gets its own render pass with a cleared depth buffer, so
overlapping viewports don't share depth.

## Cameras

`GFX_Camera` holds `projection` and `view`, plus a `scaling_mode`. Systems fill
them from other components:

- `GFX_Transform` → `view` (its inverse: the world moves opposite to the
  camera);
- `GFX_CameraOrtho {size}` → orthographic projection, `size` = half the visible
  extent along the fixed axis, in world units (pixels for sprites);
- `GFX_CameraPerspective {fov, near, far}` → perspective projection, `fov`
  along the fixed axis, in radians. The camera looks down −Z.

Both projections are built for a square view. The viewport stretches them to
its own shape (`gfx_camera_view_projection`, `src/camera/gfx_camera.c`):

- `GFX_CAMERA_SCALING_MODE_FIT_HEIGHT`: x is scaled by `1 / aspect`, the
  height stays fixed;
- `GFX_CAMERA_SCALING_MODE_FIT_WIDTH`: y is scaled by `aspect`, the width stays
  fixed.

That's exact for both projections (`tests/test_viewport.c` checks it), and one
camera can be shown in viewports of different shapes.

## Render textures

A `GFX_RenderTexture` is a texture plus a depth buffer that viewports can draw
into. The game owns it, like any texture.

```c
GFX_RenderTexture screen =
    gfx_render_texture_create(160, 120, GFX_TEXTURE_FILTER_NEAREST);

// Draw the scene into it...
ecs_set(world, scene_view, GFX_Viewport,
        {.camera = scene_cam, .texture = &screen, .rect = {0, 0, 1, 1}});
ecs_set(world, ecs_new_w_pair(world, EcsChildOf, scene_view),
        GFX_SpriteBatch, {.tag = Scene});

// ...and show it in the window as a sprite
GFX_Sprite s = gfx_sprite_create(&screen.texture, 0, 0, 160, 120);
```

- Create it after the first window: it uses the window format.
- Its color is cleared by its first view of the frame, so several viewports can
  share one texture.
- A texture can't be drawn into and sampled in the same pass. Tag the batches
  (`GFX_SpriteBatch.tag`) so the sprite showing the texture isn't drawn into
  it. `examples/engine_render_target.c` is the full example.

### Following a window's size

`gfx_render_texture_resize(&rt, width, height)` recreates the color and depth
textures in place: `rt` keeps its address, so every pointer to it stays valid,
and the sampler is kept. It does nothing for the same size or a size of 0
(minimized windows).

To keep a texture at a window's size, tag the viewport **drawing into** it
with `GfxFitSurface` and set its `surface` to that window. A system in
`GfxPreRender` resizes the texture before any view draws:

```c
ecs_set(world, scene_view, GFX_Viewport,
        {.camera = scene_cam, .texture = &screen, .surface = window,
         .rect = {0, 0, 1, 1}});
ecs_add(world, scene_view, GfxFitSurface);
```

Like Godot's `SubViewportContainer.stretch`. The size is the window size in
screen coordinates, so on a 2× screen the texture has half the window's
pixels per axis.

Materials notice the new texture by themselves (see "Shaders and materials").

## Post-processing

A post effect is a render batch that draws one fullscreen triangle with a
material sampling a render texture (`src/post/gfx_post.c`). The usual setup:
the scene viewport draws into a render texture, and a viewport without camera
draws the effect into the window (`examples/engine_post_processing.c`).

```c
GFX_Shader desaturate = gfx_shader_create("./assets/desaturate.wgsl",
                                          sizeof(float), 1);
float strength = 0.5f;
const GFX_Texture2D *textures[] = {&screen.texture};
GFX_PostEffect effect = gfx_post_effect_create(&desaturate, &strength, textures);

ecs_entity_t post_view = ecs_new(world); // no camera: identity view-projection
ecs_set(world, post_view, GFX_Viewport, {.surface = window, .rect = {0, 0, 1, 1}});
ecs_set_ptr(world, ecs_new_w_pair(world, EcsChildOf, post_view),
            GFX_PostEffect, &effect);
...
gfx_post_effect_destroy(&effect);
```

The effect shader has only `frag_main`. The engine's vertex stage
(`src/graphics/fullscreen.wgsl`) outputs the uv, (0, 0) at the top left:

```wgsl
@group(2) @binding(0) var<uniform> strength: f32;
@group(2) @binding(1) var tex: texture_2d<f32>;
@group(2) @binding(2) var tex_sampler: sampler;

@fragment
fn frag_main(@location(0) uv: vec2f) -> @location(0) vec4f {
    let color = textureSample(tex, tex_sampler, uv);
    let luma = dot(color.rgb, vec3f(0.2126, 0.7152, 0.0722));
    return vec4f(mix(color.rgb, vec3f(luma), strength), color.a);
}
```

- `GFX_PostEffect` adds its own `GFX_RenderBatch` (an `on_add` hook), so the
  entity only needs the effect.
- Chains are more render textures and viewports, ordered by `order`.
- To keep the effect at the window's size, tag the **scene** viewport (the one
  drawing into the render texture) with `GfxFitSurface`, not the effect's
  viewport.

## Sprites

`GFX_Sprite` (an atlas rect and a size in pixels) + `GFX_Transform` +
`GFX_SpriteLayer`. A `GFX_SpriteBatch` in a viewport draws them
(`src/sprite/gfx_sprite.c`):

1. collect the sprites (only those with `tag` when set);
2. sort by layer, then texture;
3. upload one instance buffer, owned by the batch;
4. one instanced draw per run of sprites sharing a texture.

The shared shader, pipeline and per-texture materials (keyed by
`const GFX_Texture2D *`) live in the `GFX_SpriteRenderer` singleton. Sprites aren't depth tested: layers decide what
is on top.

## Low-level API

The `graphics_*` examples use it directly, without the ECS.

```c
gfx_graphics_init();
GFX_Window *win = gfx_window_init(800, 600, "Example");
gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING); // before pipelines

GFX_Material material = gfx_material_create(
    gfx_shader_unlit(), &(GFX_UnlitUniform){.albedo = {1, 1, 1, 1}}, NULL);
GFX_Pipeline pipeline =
    gfx_pipeline_create(material.shader, gfx_geometry3d_layout());

while (!gfx_window_should_close(win)) {
  gfx_frame_begin();
  gfx_frame_window_begin(win);                 // acquire + clear
  gfx_frame_view_begin(win, (vec4){0, 0, 1, 1}, view_projection);
  gfx_pipeline_bind(&pipeline);
  gfx_material_bind(&material);
  gfx_draw_set_model(model);
  gfx_geometry3d_draw(&geometry);
  gfx_frame_end();                             // submit + present
  gfx_window_events_poll();
}
```

Draws only record inside a view; outside one, or when the window was skipped
(minimized), they do nothing.

### Uniforms

Every shader can read two engine uniforms:

```wgsl
@group(0) @binding(0) var<uniform> view_projection: mat4x4f; // per view
@group(1) @binding(0) var<uniform> model: mat4x4f;           // per draw
```

Both go into one per-frame arena of 4096 matrices (256-byte slots, dynamic
offsets), uploaded once in `gfx_frame_end` (`src/graphics/gfx_uniforms.c`).

### Shaders and materials

A shader is WGSL with `vert_main` and `frag_main`. It declares its material
bindings when created; a material is a shader plus values for them.

```c
// @group(2): binding 0 the uniform, texture i at 1+2i, its sampler at 2+2i
GFX_Shader shader = gfx_shader_create("./assets/lit.wgsl", sizeof(MyUniform), 1);
GFX_Material material = gfx_material_create(&shader, &uniform, textures);
GFX_Pipeline pipeline = gfx_pipeline_create(&shader, gfx_geometry3d_layout());
```

- `0, 0` means no material (no `@group(2)`).
- Missing textures (NULL or failed loads) are white.
- A material keeps pointers to its shader and its textures: they must outlive
  it and not move.
- Materials follow their textures: `gfx_material_bind` rebuilds the bind group
  when a texture's view changed (a resized render texture), keeping the
  uniform buffer. That's why it takes a non-const `GFX_Material *`.
- `gfx_shader_unlit()` is the built-in unlit shader for `GFX_Vertex3D` meshes,
  with `GFX_UnlitUniform`.
- A pipeline = shader + vertex layout + the graphics features enabled when it
  was created + the window format, so create it after the first window.
- `gfx_pipeline_fullscreen_create(&shader)` takes no vertex buffers: the vertex
  stage is the engine's fullscreen triangle, the fragment stage and material
  come from `shader`. Draw it with `gfx_draw_instanced(3, 1, 0)`.

## Compute

Storage buffers, compute shaders and dispatches, with or without a window
(`include/gfx/compute.h`, `examples/compute_basic.c`).

```c
gfx_graphics_init(); // no window needed

GFX_StorageBuffer buffer = gfx_storage_buffer_create(count * sizeof(float), false);
gfx_storage_buffer_write(&buffer, 0, values, count * sizeof(float));
GFX_ComputeShader scale =
    gfx_compute_shader_create("./assets/shaders/compute_scale.wgsl", sizeof(float), 1);

uint32_t gx, gy;
gfx_compute_groups(count, 64, &gx, &gy);
const GFX_StorageBuffer *buffers[] = {&buffer};
gfx_compute_begin();
gfx_compute_dispatch(&scale, buffers, &two, gx, gy, 1);
gfx_compute_dispatch(&scale, buffers, &three, gx, gy, 1);
gfx_compute_end();

gfx_storage_buffer_read(&buffer, 0, values, count * sizeof(float)); // blocks
```

The shader has `comp_main` and its bindings in `@group(0)`, declared when it's
created (`uniform_size`, `storage_count`):

```wgsl
@group(0) @binding(0) var<uniform> factor: f32;                      // the uniform
@group(0) @binding(1) var<storage, read_write> values: array<f32>;  // storage 0

@compute @workgroup_size(64)
fn comp_main(@builtin(workgroup_id) group: vec3u,
             @builtin(num_workgroups) groups: vec3u,
             @builtin(local_invocation_index) local: u32) {
    let i = (group.y * groups.x + group.x) * 64u + local;
    if (i >= arrayLength(&values)) { return; }
    values[i] = values[i] * factor;
}
```

- **Storage buffers are bytes.** The WGSL describes their structure; sizes and
  offsets are in bytes, multiples of 4. Created with `vertex`, a storage buffer
  can be drawn: `gfx_storage_buffer_as_vertex(&buffer)` gives the same buffer as
  a `GFX_VertexBuffer`, described by the drawing pipeline's vertex layout.
  WGSL pads `vec3f` to 16 bytes and vertex layouts pack tightly, so use `f32`,
  `vec2f` and `vec4f` fields in structs you draw.
- **A session records, then submits.** `gfx_compute_dispatch` only records the
  dispatch and copies its uniform (at most 256 bytes); `gfx_compute_end`
  uploads every uniform once, encodes one compute pass and submits it.
  Dispatches in a pass see each other's writes, so a chain of dispatches on one
  buffer is fine. Shaders and buffers must stay alive until `gfx_compute_end`.
- **With rendering:** call the session before `GfxEndRender` (for example in
  `GfxPreRender`); its submit runs before the frame's, so the frame can draw
  the result.
- **Workgroups:** `gfx_compute_groups(count, workgroup_size, &x, &y)` covers
  `count` invocations and splits into 2D past the 65535-per-dimension limit;
  the shader rebuilds its index as above and skips indices past the end.
- **Readback** blocks until the GPU is done (through a staging buffer, ASYNCIFY
  in the browser). It waits for everything submitted before it, compute
  included.

### Limits

The device gets WebGPU's default limits (256 MiB buffers, 128 MiB storage
bindings). Ask for more only when needed, before `gfx_graphics_init`:

```c
GFX_GraphicsLimits adapter = gfx_graphics_adapter_limits(); // works before init
uint64_t size = sizeof(float) * 2 * (1ull << qubits);
gfx_graphics_require_limits(&(GFX_GraphicsLimits){
    .max_buffer_size = size, .max_storage_binding_size = size});
gfx_graphics_init(); // fails, with a log, if the adapter can't provide them
```

`gfx_graphics_limits()` gives the device's limits after init. A storage buffer
bigger than `max_buffer_size` isn't created (an error is logged).

## Rules

- **Lifetimes.** Sprites, materials and viewports hold raw pointers (textures,
  shaders, render textures). What they point at must outlive them and not
  move, so don't point into component storage.
- **Names.** Modules don't call `ecs_set_name_prefix`, so entities keep the
  names used in C (`GfxFitSurface`) and query strings find them.
- **Per-view data.** Batches run once per viewport. A buffer written each frame
  must belong to the batch, or every view sees the last write.
- **Systems.** Don't keep pointers to component storage across phases: flecs
  can move it. Systems that touch WebGPU or GLFW run on the main thread.
- **WebGPU structs** start from their `_INIT` macro: most have a `nextInChain`
  pointer that Dawn follows, garbage crashes.
- **Browser.** No blocking loops (use `gfx_run`), no extra blocking waits, one
  window. Apps package their assets with `--preload-file`.
- **Shaders the engine owns** live next to their code and are compiled in with
  `#embed` (`src/sprite/sprite.wgsl`, `src/graphics/unlit.wgsl`,
  `src/graphics/fullscreen.wgsl`).
