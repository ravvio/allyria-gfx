# GFX Engine

A small C99 game engine that runs natively and in the browser.

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

## Build

Requirements: CMake 3.22+ and a C/C++ compiler. The dependencies (GLFW, Dawn,
cglm, glfw3webgpu) are downloaded by CMake.

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

Examples are written to `build/Debug/<arch>/` (e.g. `arm64` or `x86_64`) and
must be run from there, because assets are loaded from `./assets`:

```sh
cd build/Debug/<arch>
./engine_sprites
```

The assets are copied at configure time, so re-run `cmake -B build` after
changing anything in `assets/`.

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
python3 -m http.server 8000 --bind 127.0.0.1 -d build/web/Debug/x86
```

Then open <http://localhost:8000/engine_sprites.html> in a browser with WebGPU
support (recent Chrome, Edge or Safari).
