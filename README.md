# GFX Engine

A small C23 game engine library that runs natively and in the browser.

Using:
- [flecs](https://github.com/SanderMertens/flecs) for the ECS
- [WebGPU](https://www.w3.org/TR/webgpu/) for rendering, through
  [Dawn](https://dawn.googlesource.com/dawn) natively and emdawnwebgpu in the
  browser
- [GLFW](https://www.glfw.org/) for windows and input, with
  [glfw3webgpu](https://github.com/eliemichel/glfw3webgpu) for the WebGPU surface
- [cglm](https://github.com/recp/cglm) for math
- [stb_image and stb_ds](https://github.com/nothings/stb) for image loading and
  dynamic arrays

## Use it in a project

Add the engine to your CMake project, for example with `FetchContent` or as a
subdirectory, and link the `gfx` target:

```cmake
add_subdirectory(allyria-gfx)
target_link_libraries(my_game PRIVATE gfx)
```

The target brings its include directory and dependencies with it, so headers
are included as `<gfx/...>`:

```c
#include <gfx/gfx.h>
```

Your game ships its own assets. The engine needs no files at runtime.

## Build

Requirements: CMake 3.22+ and a C/C++ compiler with C23 `#embed` support
(Clang 19+ or GCC 15+, MSVC isn't supported). The dependencies (GLFW, Dawn,
glfw3webgpu) are downloaded by CMake.

When built on its own, the project also builds the examples and the tests.

### Native

Configure the build. It defaults to Debug and downloads the dependencies on the
first run:

```sh
cmake -B build
```

Build the library, the examples and the tests, compiling files in parallel:

```sh
cmake --build build --parallel
```

Run the tests:

```sh
ctest --test-dir build
```

Examples are written to `build/examples/` and must be run from there, because
they load their assets from `./assets`:

```sh
cd build/examples
./engine_sprites
```

The examples' assets are copied at configure time, so re-run `cmake -B build`
after changing anything in `examples/assets/`.

### Browser

The browser build needs [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)
4.0.10 or newer. Emscripten itself needs Python 3.10 or newer: if the `python3`
on your `PATH` is older, point `EMSDK_PYTHON` to a newer interpreter.

Configure the build in `build/web` with Emscripten's CMake wrapper:

```sh
emcmake cmake -B build/web
```

Build the examples as `.html` pages:

```sh
cmake --build build/web --parallel
```

Only the `engine_*` examples are built for the browser, the `graphics_*` ones
run their own blocking loop.

## Serve

The pages must be served over HTTP, opening the files directly doesn't work.
Any static file server works, for example Python's (bound to localhost only):

```sh
python3 -m http.server 8000 --bind 127.0.0.1 -d build/web/examples
```

Then open <http://localhost:8000/engine_sprites.html> in a browser with WebGPU
support (recent Chrome, Edge or Safari).
