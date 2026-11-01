# AGENTS.md

C23 game engine **library** (`gfx` target): **flecs** (ECS) + **WebGPU** (Dawn natively, emdawnwebgpu in the browser) + **GLFW** + **cglm** + **stb_image / stb_ds**.

## Build & run

```sh
cmake -B build                 # Debug by default; fetches GLFW, Dawn, glfw3webgpu
cmake --build build --parallel
ctest --test-dir build
cd build/examples && ./engine_sprites
```

- Browser build: `emcmake cmake -B build/web` then `cmake --build build/web --parallel`. Output is `build/web/examples/*.html`; serve it over HTTP (see `README.md`). Emscripten needs Python 3.10+: set `EMSDK_PYTHON` if `python3` is older.
- The root `CMakeLists.txt` builds only the library. Examples (`GFX_BUILD_EXAMPLES`) and tests (`GFX_BUILD_TESTS`) default to on only when this is the top-level project, so a game can `add_subdirectory` / `FetchContent` the engine without them.
- GPU-free logic has CTest checks in `tests/`. Add one when you write non-trivial logic that runs without a device.
- Everything else is verified by building and running the affected examples (`examples/*.c`, one executable each, registered with `add_example()` in `examples/CMakeLists.txt`). WebGPU errors print as `[e] WebGPU error`.
- New `.c` files in `src/` must be added to `GFXSOURCES` in `CMakeLists.txt`.

## The library and its users

`examples/` is built exactly like a game using the engine. Keep it that way: if an example needs something extra to build or run, the library is missing it.

- **A consumer only does `target_link_libraries(game PRIVATE gfx)` and `#include <gfx/...>`.** Everything else comes from the `gfx` target's usage requirements:
  - the PUBLIC include dir `include/` (only `gfx/` lives there);
  - PUBLIC `webgpu`, `flecs` and `cglm`, each bringing its own include dir. stb is PRIVATE: it never appears in public headers;
  - the Emscripten compile and link flags.
- **The library ships no runtime files.** Shaders the engine owns live next to their code (`src/sprite/sprite.wgsl`) and are compiled in with C23 `#embed`. Don't make the engine load files from the app's working directory.
- **Assets belong to the app.** Each consumer ships its own (`examples/assets/` is copied next to the examples and packaged for the browser by `examples/CMakeLists.txt`).
- **Use `CMAKE_CURRENT_SOURCE_DIR` / `CMAKE_CURRENT_BINARY_DIR`** in the library's CMake, never `CMAKE_SOURCE_DIR` / `CMAKE_BINARY_DIR`: those point at the consumer's project when the engine is a subdirectory. Settings that affect the whole build (build type, `compile_commands.json`) only apply when top-level.

## Layout

| Path | What |
|---|---|
| `include/gfx/gfx.h` | Includes every feature header, what the examples use |
| `include/gfx/core.h` | `GFX_Error`, color types, cglm (with `CGLM_FORCE_DEPTH_ZERO_TO_ONE`) |
| `include/gfx/graphics.h` | Device init/terminate, graphics features |
| `include/gfx/window.h` | Window, frame begin/end, clear color |
| `include/gfx/render.h` | Buffers, vertex layout, shaders, pipelines, view-projection/model uniforms, draws, 2D/3D geometry |
| `include/gfx/texture.h`, `material.h` | Textures with samplers; material layouts and materials |
| `include/gfx/engine.h` | Settings, `gfx_init`/`gfx_terminate`/`gfx_run`, surface, render phases, viewports and render batches |
| `include/gfx/transform.h`, `camera.h`, `sprite.h` | ECS modules |
| `src/gfx_internal.h` | Private to the library: shared WebGPU state (`_gfx`), GLFW, `GLFWCall`, internal init functions |
| `src/graphics/gfx_window.c` | WebGPU instance/adapter/device, window + surface + depth buffer, frame begin/end, views (one render pass each) |
| `src/graphics/gfx_buffers.c` | Vertex/index buffers, vertex layout, draw calls |
| `src/graphics/gfx_shader.c` | WGSL shader modules, render pipelines |
| `src/graphics/gfx_uniforms.c` | Shared bind group/pipeline layouts, per-frame matrix arena (view-projections and models) |
| `src/graphics/gfx_texture.c` | RGBA8 textures (from pixels or `stb_image`) with their sampler, CPU mipmaps |
| `src/graphics/gfx_material.c` | Material layouts (defined by the caller), materials, white fallback texture |
| `src/graphics/gfx_geometry2d.c`, `gfx_geometry3d.c` | Meshes (`GFX_Vertex2D` / `GFX_Vertex3D`), quad and cuboid shapes, their vertex layouts |
| `src/renderer/` | Render phases & frame systems, surface (window + DPI), viewport loop |
| `src/sprite/gfx_sprite.c`, `sprite.wgsl` | Instanced sprites, drawn by a `GFX_SpriteBatch` in each viewport: query, sort by (layer, texture), one instance upload, one draw per texture run. The shader is embedded with `#embed` |
| `src/camera/`, `src/sprite/`, `src/gfx_*.c` | flecs modules |
| `examples/` | Example programs and their own `CMakeLists.txt`, built like a game would build |
| `examples/assets/` | The examples' shaders (WGSL, `vs_main` / `fs_main`) and textures |
| `tests/` | CTest checks for GPU-free logic |
| `vendor/{flecs,cglm,stb}` | Vendored third-party code, one CMake target each (`flecs`, `cglm`, `stb`) — don't edit |

## Conventions

- Types `GFX_PascalCase`, functions `gfx_snake_case`, enums `GFX_UPPER_CASE`. Integer types from `<stdint.h>` (`uint32_t`), not BSD `u_int32_t` (missing on Emscripten's libc).
- flecs modules: `void Gfx<Name>Import(ecs_world_t *)` using `ECS_MODULE` + `ecs_set_name_prefix(world, "Gfx")`; imported with `ECS_IMPORT(world, Gfx<Name>)`.
- Components: `extern ECS_COMPONENT_DECLARE(T)` in the header, `ECS_COMPONENT_DECLARE(T)` in the `.c`, `ECS_COMPONENT_DEFINE` in the import function. Components owning GPU/heap resources get `ECS_DTOR` + `ECS_MOVE` hooks (see `gfx_surface.c`, `gfx_sprite.c`).
- System callbacks are named `_gfx_sys_<what>`. Systems must not keep pointers to component storage across phases (flecs can move it): copy what they need, as the sprite renderer does.
- Includes: consumers (examples) use `#include <gfx/...>`. Inside the library, sources and headers include each other with relative paths (`"../../include/gfx/gfx.h"`, `"../gfx_common.h"`).
- C23, standard mode (`C_EXTENSIONS OFF`): no compiler extensions, so the code also builds on Emscripten's libc.
- The backend boundary is `gfx_graphics.h` + `src/graphics/`. Modules above it (`renderer/`, `sprite/`) only use that API, never `wgpu*` calls directly.
- Resource structs are returned by value from `*_create` / `*_load` and released with `*_destroy(const T *)`.
- `*_bind` / `gfx_draw_*` record into the current frame's render pass. Call them between `gfx_frame_begin` and `gfx_frame_end`; they do nothing when the frame was skipped.
- Pipelines (`gfx_pipeline_create`) read the enabled graphics features and the surface format when created: enable features and create the window first. ECS modules can be imported before the game sets `GFX_Surface`, so they create their pipelines on first draw.
- Uniforms follow a fixed contract. Pipeline layouts are always explicit (never `'auto'`), and groups 0 and 1 are shared by every pipeline:
  - `@group(0) @binding(0) var<uniform> view_projection: mat4x4f;` set per view with `gfx_frame_view_begin(rect, view_projection)`, identity until a view begins;
  - `@group(1) @binding(0) var<uniform> model: mat4x4f;` set per draw with `gfx_draw_set_model`;
  - both go into one 4096-slot per-frame arena (dynamic offsets), uploaded once in `gfx_frame_end`;
  - `@group(2)` is the material. Its layout is defined by the caller with `gfx_material_layout_create(uniform_size, texture_count)` and passed to `gfx_pipeline_create` (NULL = no group 2):
    - binding 0 is the uniform (only if the size is > 0);
    - texture *i* is at binding `1+2i`, its sampler at `2+2i`;
    - NULL or failed textures fall back to white;
    - the engine doesn't name material fields; the shader and the caller's uniform struct do.
  - Never `writeBuffer` per-draw data into one shared buffer region: every draw would see the last value. Renderers run once per viewport, so buffers they write each frame belong to the renderer entity (each `GFX_SpriteBatch` has its own instance buffer).
- Instanced data (sprites) uses a vertex buffer with `GFX_VertexLayout.step = WGPUVertexStepMode_Instance` and `gfx_draw_instanced`.
- Coordinates: clip depth is `[0, 1]` (`CGLM_FORCE_DEPTH_ZERO_TO_ONE` is defined in `gfx_common.h`, so cglm projections match), front faces are counter-clockwise, texture origin is top-left.
- Logging: `fprintf(stderr, "[i] ...")` for info, `"[e] ..."` for errors. Per-frame logs only under `#ifdef GFX_VERBOSE`.
- Errors: return `GFX_Error` (`GFX_ERR_OK` == 0). Wrap GLFW calls in `GLFWCall(...)` (compiled out with `GFX_NDEBUG`). WebGPU errors come from the device's uncaptured-error callback.
- Dynamic arrays and hashmaps use `stb_ds`. Math uses cglm (`mat4`, `vec3`, `glm_*`).
- Design decisions based on outside sources (specs, articles, other engines): cite them and add them to `SOURCES.md` (references and a short note on consensus).
- Formatting: `.clang-format` (LLVM style, 2-space indent, 80 cols). Format the files you touch. Vendored code is listed in `.clang-format-ignore` (read by clang-format 18+).

## Frame / ECS flow

`gfx_run(world)` calls `ecs_progress()` every frame (a loop natively, `requestAnimationFrame` in the browser), which runs:
1. `EcsOnLoad`: sync `GFX_Surface` with the window, so its size is current for the whole frame (gameplay included).
2. `EcsPreUpdate`: quit if the window should close or never opened.
3. `EcsOnUpdate` / `EcsPostUpdate`: gameplay.
4. Render phases (defined in `gfx_renderer.c`, chained after `EcsOnStore`):
   `GfxPreRender` (camera systems: model from `GFX_Transform`, view from `GFX_CameraPixel`, a square projection from `GFX_CameraOrtho`) → `GfxStartRender` (`gfx_frame_begin`: acquire surface texture, begin cleared render pass) → `GfxRenderObjects` (the viewport loop, below) → `GfxRenderGizmos` → `GfxRenderGUI` → `GfxEndRender` (`gfx_frame_end`: upload the matrix arena, submit, present) → `GfxPostRender` (poll events).

The game opens the window by setting the `GFX_Surface` singleton (title, width, height) after `gfx_init`; its `on_set` hook creates the window. The component has flecs' `Singleton` trait, so systems take it as a plain `GFX_Surface` term and only run once it exists.

Viewports follow the flecs engine's `RenderView` (see `SOURCES.md`). A `GFX_Viewport` entity points at a camera and draws into a rect of the window. Its renderers are child entities with `GFX_RenderBatch` (a draw function), drawn in the order they were added (`EcsOrderedChildren`). The viewport loop, sorted by `order`:
1. view-projection = aspect fit (from the camera's `scaling_mode` and the viewport's aspect) × square projection × view × inverse(camera transform);
2. `gfx_frame_view_begin(rect, view_projection)`;
3. each renderer's `draw`.

Draw code goes in a render batch, not in a system: systems run once per frame, batches once per viewport.

Systems that depend on each other go in different phases, not in declaration order within one phase: flecs doesn't guarantee that order.

Every frame's render pass has a `Depth24Plus` depth attachment, owned by the window and recreated on resize. Every pipeline therefore declares a depth state; without `GFX_GRAPHICS_FEATURE_DEPTH_TESTING` it uses compare `Always` with writes off. Each view after the first starts a new render pass (color loaded, depth cleared), so overlapping viewports don't share depth.

WebGPU objects (instance, adapter, device, queue, current frame's encoder/pass) live in the `_gfx` struct (`gfx_wgpu.h`, defined in `gfx_window.c`). A frame can be skipped (window minimized, surface outdated): `gfx_frame_end` then does nothing, and draw code must handle a NULL render pass.

## Targets

Native is the default. Configuring with `emcmake cmake` builds for the browser (`EMSCRIPTEN` is set).

- Both use the same Dawn release (`DAWN_TAG` / `DAWN_REV` in `CMakeLists.txt`):
  - **Native:** Dawn's prebuilt archive, pinned with a SHA256 per platform (target `dawn::webgpu_dawn`). To upgrade, change the tag/revision and all hashes.
  - **Browser:** Dawn's `emdawnwebgpu` remote port from the same release, also pinned by hash.
- Only use the standard `webgpu.h` API. Code for one target goes behind `#ifdef __EMSCRIPTEN__` / `#ifndef __EMSCRIPTEN__`, kept inside `src/graphics/` or `gfx_run`.
- Engine code must work in the browser:
  - **No blocking main loop.** Use `gfx_run`. The `graphics_*` examples have their own loop, so only the `engine_*` examples are built for the browser.
  - **Adapter/device requests are async.** Init waits on them with `wgpuInstanceWaitAny` (instance feature `TimedWaitAny`), which needs `-sASYNCIFY` in the browser. Don't add other blocking waits.
  - **No `wgpuSurfacePresent` / `wgpuInstanceProcessEvents`** in the browser (it presents on its own).
- Browser flags live on the library's targets, so consumers get them by linking `gfx`:
  - `glfw`: Emscripten's `contrib.glfw3` port; the built-in `-sUSE_GLFW=3` is GLFW 3.2 and lacks `glfwGetError`.
  - `webgpu`: the emdawnwebgpu port, `-sASYNCIFY`, and `-sDEFAULT_TO_CXX` (emdawnwebgpu needs the C++ runtime).
  - `flecs`: `-sSTACK_SIZE=1mb` (it overflows the default 64 KB in `ecs_init`) and `-sALLOW_MEMORY_GROWTH=1`.
- Apps package their own assets with `--preload-file <dir>@/assets`, so `./assets/...` paths work unchanged. If several flags repeat, use `SHELL:` in `target_link_options` or CMake drops the duplicates.
- **Threads:** the engine must work with any flecs thread count but must not require threads. Games can call `ecs_set_threads` (native only).
  - Systems that touch WebGPU or GLFW run on the main thread: never mark them `multi_threaded`.
  - Browser builds are single-threaded; threads there would need `-pthread` plus COOP/COEP headers.

## Gotchas

- flecs is built from the vendored `include/flecs/flecs.c`; header and source must stay the same version (a mismatch crashes at startup). Upgrade both files together.
- cglm is header-only and used from the vendored `include/cglm`.
- `STB_IMAGE_IMPLEMENTATION` / `STB_DS_IMPLEMENTATION` are defined only in `src/gfx.c`.
- Warnings (`-Wall -Wextra -pedantic`) are only enabled on example targets, not on the `gfx` library.
- Dawn is a C++ static library, so the project enables `CXX` for linking. Engine code stays C23.
- `#embed` needs Clang 19+ or GCC 15+. MSVC doesn't support it, so the engine can't be built with MSVC (clang-cl works).
- Only Dawn's Release archives are downloaded: Windows Debug builds would need the Debug archive (MSVC runtime mismatch).
- WebGPU has no `generateMipmap`.
  - **Linear textures** get a full mip chain built on the CPU at creation (2×2 box filter). Averaging non-premultiplied alpha can darken transparent edges in the smaller mips.
  - **Nearest textures** keep a single level, so atlas cells don't bleed.
  - Move to GPU generation if load time matters or render-to-texture is added.
- The uniform arena holds 4096 draws per frame; past that, extra draws reuse the last model and an error is logged once per frame.
- Editors may show `std::is_empty_v` errors in headers that include `flecs.h`: the editor parses the header as C++. It's not a build error.
