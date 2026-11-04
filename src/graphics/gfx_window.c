#include "gfx_internal.h"
#include <gfx/graphics.h>
#include <gfx/window.h>
#include <glfw3webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GLFWwindow *glfw;
  WGPUSurface surface;
  WGPUTextureFormat format;
  // Depth buffer, same size as the surface
  WGPUTexture depth;
  WGPUTextureView depth_view;
  // Size the surface is configured with (framebuffer pixels)
  int width;
  int height;
  // Image drawn this frame, NULL when the window wasn't begun
  WGPUTexture frame_texture;
  WGPUTextureView frame_view;
} _GFX_Window;

static void _gfx_window_depth_release(_GFX_Window *window) {
  if (window->depth) {
    wgpuTextureViewRelease(window->depth_view);
    wgpuTextureRelease(window->depth);
    window->depth = NULL;
    window->depth_view = NULL;
  }
}

// *** WINDOW ***

WGPUTexture gfx_depth_texture_create(uint32_t width, uint32_t height) {
  WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
  desc.usage = WGPUTextureUsage_RenderAttachment;
  desc.dimension = WGPUTextureDimension_2D;
  desc.size = (WGPUExtent3D){width, height, 1};
  desc.format = GFX_DEPTH_FORMAT;
  return wgpuDeviceCreateTexture(_gfx.device, &desc);
}

// (Re)configure the surface to the framebuffer size, fails when minimized
static bool _gfx_window_configure(_GFX_Window *window) {
  int w, h;
  GLFWCall(glfwGetFramebufferSize(window->glfw, &w, &h));
  if (w == 0 || h == 0) {
    return false;
  }

  WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
  config.device = _gfx.device;
  config.format = window->format;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.width = w;
  config.height = h;
  config.presentMode = WGPUPresentMode_Fifo;
  wgpuSurfaceConfigure(window->surface, &config);

  // Recreate the depth buffer at the new size
  _gfx_window_depth_release(window);
  window->depth = gfx_depth_texture_create(w, h);
  window->depth_view = wgpuTextureCreateView(window->depth, NULL);

  window->width = w;
  window->height = h;
  return true;
}

GFX_Window *gfx_window_init(int w, int h, const char *window_name) {
  // No GL context, WebGPU draws through the surface
  GLFWCall(glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API));

  // Create a window
  GLFWCall(GLFWwindow *glfw = glfwCreateWindow(w, h, window_name, NULL, NULL));
  if (!glfw) {
    return NULL;
  }

  _GFX_Window *window = calloc(1, sizeof(_GFX_Window));
  window->glfw = glfw;

  // Create the surface and pick its preferred format
  window->surface = glfwCreateWindowWGPUSurface(_gfx.instance, glfw);
  WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
  if (!window->surface ||
      wgpuSurfaceGetCapabilities(window->surface, _gfx.adapter, &caps) !=
          WGPUStatus_Success ||
      caps.formatCount == 0) {
    fprintf(stderr, "[e] failed to create WebGPU surface\n");
    gfx_window_destroy(window);
    return NULL;
  }
  // Every window uses the first window's format, pipelines are built for it
  if (_gfx.surface_format == WGPUTextureFormat_Undefined) {
    _gfx.surface_format = caps.formats[0];
  }
  window->format = _gfx.surface_format;
  bool supported = false;
  for (size_t i = 0; i < caps.formatCount; i++) {
    supported |= caps.formats[i] == window->format;
  }
  wgpuSurfaceCapabilitiesFreeMembers(caps);
  if (!supported) {
    fprintf(stderr, "[e] window doesn't support the first window's format\n");
    gfx_window_destroy(window);
    return NULL;
  }

  _gfx_window_configure(window);

  return window;
}

// Destroy the given window
void gfx_window_destroy(GFX_Window *window) {
  _GFX_Window *win = window;
  _gfx_window_depth_release(win);
  if (win->surface) {
    if (win->width > 0) {
      wgpuSurfaceUnconfigure(win->surface);
    }
    wgpuSurfaceRelease(win->surface);
  }
  GLFWCall(glfwDestroyWindow(win->glfw));
  free(win);
}

// Get the size of the window
void gfx_window_size(GFX_Window *window, int *w, int *h) {
  GLFWCall(glfwGetWindowSize(((_GFX_Window *)window)->glfw, w, h));
}

// Set the size of the window
void gfx_window_size_set(GFX_Window *window, int w, int h) {
  GLFWCall(glfwSetWindowSize(((_GFX_Window *)window)->glfw, w, h));
}

// Get the content scale of the window
void gfx_window_scale(GFX_Window *window, float *sx, float *sy) {
  GLFWCall(glfwGetWindowContentScale(((_GFX_Window *)window)->glfw, sx, sy));
}

// Should the window be closed
int gfx_window_should_close(GFX_Window *window) {
  GLFWCall(int result = glfwWindowShouldClose(((_GFX_Window *)window)->glfw));
  return result;
}

// Poll window events
void gfx_window_events_poll() { GLFWCall(glfwPollEvents()); }

// *** FRAME ***

// Set the color the frame is cleared with
void gfx_color_buffer_set_clear(const GFX_ColorRGBA color) {
  _gfx.clear_color = (WGPUColor){color[0], color[1], color[2], color[3]};
}

// Begin a render pass into color and depth
static void _gfx_pass_begin(WGPUTextureView color_view,
                            WGPUTextureView depth_view, WGPULoadOp color_load) {
  WGPURenderPassColorAttachment color = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  color.view = color_view;
  color.loadOp = color_load;
  color.storeOp = WGPUStoreOp_Store;
  color.clearValue = _gfx.clear_color;

  WGPURenderPassDepthStencilAttachment depth =
      WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
  depth.view = depth_view;
  depth.depthLoadOp = WGPULoadOp_Clear;
  depth.depthStoreOp = WGPUStoreOp_Discard;
  depth.depthClearValue = 1.0f;

  WGPURenderPassDescriptor pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  pass_desc.colorAttachmentCount = 1;
  pass_desc.colorAttachments = &color;
  pass_desc.depthStencilAttachment = &depth;
  _gfx.pass = wgpuCommandEncoderBeginRenderPass(_gfx.encoder, &pass_desc);
}

static void _gfx_pass_end(void) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderEnd(_gfx.pass);
    wgpuRenderPassEncoderRelease(_gfx.pass);
    _gfx.pass = NULL;
  }
}

static uint32_t _gfx_viewport_edge(float t, uint32_t size) {
  float px = roundf(t * (float)size);
  if (px <= 0.0f) {
    return 0;
  }
  return px >= (float)size ? size : (uint32_t)px;
}

// Pixel position and size of a viewport, clamped to the image
void gfx_viewport_pixels(const float rect[4], uint32_t width, uint32_t height,
                         uint32_t out[4]) {
  uint32_t x0 = _gfx_viewport_edge(rect[0], width);
  uint32_t x1 = _gfx_viewport_edge(rect[0] + rect[2], width);
  uint32_t y0 = _gfx_viewport_edge(rect[1], height);
  uint32_t y1 = _gfx_viewport_edge(rect[1] + rect[3], height);
  out[0] = x0;
  out[1] = y0;
  out[2] = x1 > x0 ? x1 - x0 : 0;
  out[3] = y1 > y0 ? y1 - y0 : 0;
}

// Begin a view in a new pass, so views don't share depth
static bool _gfx_view_begin(WGPUTextureView color, WGPUTextureView depth,
                            uint32_t width, uint32_t height, WGPULoadOp load,
                            const vec4 rect, mat4 view_projection) {
  uint32_t px[4];
  gfx_viewport_pixels(rect, width, height, px);
  if (px[2] == 0 || px[3] == 0) {
    return false;
  }

  _gfx_pass_end();
  _gfx_pass_begin(color, depth, load);
  wgpuRenderPassEncoderSetViewport(_gfx.pass, (float)px[0], (float)px[1],
                                   (float)px[2], (float)px[3], 0.0f, 1.0f);
  wgpuRenderPassEncoderSetScissorRect(_gfx.pass, px[0], px[1], px[2], px[3]);
  gfx_uniforms_view_begin(view_projection);
  return true;
}

// Draw the next calls into rect of window with view_projection
bool gfx_frame_view_begin(GFX_Window *window, const vec4 rect,
                          mat4 view_projection) {
  _GFX_Window *win = window;
  if (!win->frame_view) {
    return false;
  }
  return _gfx_view_begin(win->frame_view, win->depth_view, win->width,
                         win->height, WGPULoadOp_Load, rect, view_projection);
}

// Draw the next calls into rect of texture with view_projection
bool gfx_frame_view_begin_texture(GFX_RenderTexture *texture, const vec4 rect,
                                  mat4 view_projection) {
  if (!_gfx.encoder || !texture->texture.texture) {
    return false;
  }
  // Cleared by its first view of the frame
  WGPULoadOp load =
      texture->cleared_frame == _gfx.frame ? WGPULoadOp_Load : WGPULoadOp_Clear;
  if (!_gfx_view_begin(texture->texture.view, texture->depth_view,
                       texture->texture.width, texture->texture.height, load,
                       rect, view_projection)) {
    return false;
  }
  texture->cleared_frame = _gfx.frame;
  return true;
}

// Begin a frame, where every window and texture is drawn
void gfx_frame_begin(void) {
  _gfx.frame++;
  _gfx.window_count = 0;
  _gfx.encoder = wgpuDeviceCreateCommandEncoder(_gfx.device, NULL);
  gfx_uniforms_frame_begin();
}

// Draw window this frame, false when it can't be drawn
bool gfx_frame_window_begin(GFX_Window *window) {
  _GFX_Window *win = window;
  if (!_gfx.encoder || win->frame_view) {
    return win->frame_view != NULL;
  }
  if (_gfx.window_count == GFX_MAX_WINDOWS) {
    fprintf(stderr, "[e] more than %d windows in a frame\n", GFX_MAX_WINDOWS);
    return false;
  }

  // Follow window resizes, skip the window while minimized
  int w, h;
  GLFWCall(glfwGetFramebufferSize(win->glfw, &w, &h));
  if ((w != win->width || h != win->height) && !_gfx_window_configure(win)) {
    return false;
  }

  WGPUSurfaceTexture surface_texture = WGPU_SURFACE_TEXTURE_INIT;
  wgpuSurfaceGetCurrentTexture(win->surface, &surface_texture);
  if (surface_texture.status !=
          WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surface_texture.status !=
          WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
    // Outdated or lost, reconfigure and skip this frame
    if (surface_texture.texture) {
      wgpuTextureRelease(surface_texture.texture);
    }
    _gfx_window_configure(win);
    return false;
  }

  win->frame_texture = surface_texture.texture;
  win->frame_view = wgpuTextureCreateView(win->frame_texture, NULL);
  _gfx.windows[_gfx.window_count++] = win;

  // Clear it
  _gfx_pass_end();
  _gfx_pass_begin(win->frame_view, win->depth_view, WGPULoadOp_Clear);
  _gfx_pass_end();
  return true;
}

// Finish the frame and show every window drawn in it
void gfx_frame_end(void) {
  if (!_gfx.encoder) {
    return;
  }
  _gfx_pass_end();

  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(_gfx.encoder, NULL);
  // Queue writes land before the submitted commands run
  gfx_uniforms_frame_end();
  wgpuQueueSubmit(_gfx.queue, 1, &commands);
  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(_gfx.encoder);
  _gfx.encoder = NULL;

  for (uint32_t i = 0; i < _gfx.window_count; i++) {
    _GFX_Window *win = _gfx.windows[i];
#ifndef __EMSCRIPTEN__
    // The browser presents on its own
    wgpuSurfacePresent(win->surface);
#endif
    wgpuTextureViewRelease(win->frame_view);
    wgpuTextureRelease(win->frame_texture);
    win->frame_view = NULL;
    win->frame_texture = NULL;
  }
  _gfx.window_count = 0;

#ifndef __EMSCRIPTEN__
  wgpuInstanceProcessEvents(_gfx.instance);
#endif
}
