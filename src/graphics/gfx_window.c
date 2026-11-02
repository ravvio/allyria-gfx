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
} _GFX_Window;

static void _gfx_window_depth_release(_GFX_Window *window) {
  if (window->depth) {
    wgpuTextureViewRelease(window->depth_view);
    wgpuTextureRelease(window->depth);
    window->depth = NULL;
    window->depth_view = NULL;
  }
}

_GFX_State _gfx;

// *** CALLBACKS ***

static void _gfx_wgpu_log(const char *what, WGPUStringView message) {
  size_t length =
      message.length == WGPU_STRLEN ? strlen(message.data) : message.length;
  fprintf(stderr, "[e] %s\n| %.*s\n", what, (int)length, message.data);
}

static void _gfx_on_adapter(WGPURequestAdapterStatus status,
                            WGPUAdapter adapter, WGPUStringView message,
                            void *userdata1, void *userdata2) {
  (void)userdata2;
  if (status != WGPURequestAdapterStatus_Success) {
    _gfx_wgpu_log("WebGPU adapter request failed", message);
  }
  *(WGPUAdapter *)userdata1 = adapter;
}

static void _gfx_on_device(WGPURequestDeviceStatus status, WGPUDevice device,
                           WGPUStringView message, void *userdata1,
                           void *userdata2) {
  (void)userdata2;
  if (status != WGPURequestDeviceStatus_Success) {
    _gfx_wgpu_log("WebGPU device request failed", message);
  }
  *(WGPUDevice *)userdata1 = device;
}

static void _gfx_on_error(WGPUDevice const *device, WGPUErrorType type,
                          WGPUStringView message, void *userdata1,
                          void *userdata2) {
  (void)device, (void)type, (void)userdata1, (void)userdata2;
  _gfx_wgpu_log("WebGPU error", message);
}

static void _gfx_on_device_lost(WGPUDevice const *device,
                                WGPUDeviceLostReason reason,
                                WGPUStringView message, void *userdata1,
                                void *userdata2) {
  (void)device, (void)userdata1, (void)userdata2;
  // Destroyed / cancelled happen on a normal shutdown
  if (reason != WGPUDeviceLostReason_Destroyed &&
      reason != WGPUDeviceLostReason_CallbackCancelled) {
    _gfx_wgpu_log("WebGPU device lost", message);
  }
}

// Block until the future completes (on web this needs ASYNCIFY)
static void _gfx_wait(WGPUFuture future) {
  WGPUFutureWaitInfo info = {.future = future};
  wgpuInstanceWaitAny(_gfx.instance, 1, &info, UINT64_MAX);
}

// *** GRAPHICS ***

// Initialize the gfx graphics library
GFX_Error gfx_graphics_init() {
  // Initialize GLFW
  if (glfwInit() == GLFW_FALSE) {
    gfx_glfw_errors_check();
    return GFX_ERR_GLFW;
  }
  fprintf(stderr, "[i] status: Using GLFW %s\n", glfwGetVersionString());

  // Create the instance, TimedWaitAny lets us wait on adapter/device requests
  WGPUInstanceFeatureName instance_features[] = {
      WGPUInstanceFeatureName_TimedWaitAny,
  };
  WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
  instance_desc.requiredFeatureCount = 1;
  instance_desc.requiredFeatures = instance_features;
  _gfx.instance = wgpuCreateInstance(&instance_desc);
  if (!_gfx.instance) {
    fprintf(stderr, "[e] failed to create WebGPU instance\n");
    return GFX_ERR_WGPU;
  }

  // Request the adapter
  WGPURequestAdapterCallbackInfo adapter_cb =
      WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
  adapter_cb.mode = WGPUCallbackMode_WaitAnyOnly;
  adapter_cb.callback = _gfx_on_adapter;
  adapter_cb.userdata1 = &_gfx.adapter;
  _gfx_wait(wgpuInstanceRequestAdapter(_gfx.instance, NULL, adapter_cb));
  if (!_gfx.adapter) {
    return GFX_ERR_WGPU;
  }

  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
  wgpuAdapterGetInfo(_gfx.adapter, &info);
  fprintf(stderr, "[i] status: Using WebGPU adapter %.*s\n",
          (int)info.device.length, info.device.data);
  wgpuAdapterInfoFreeMembers(info);

  // Request the device
  WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
  device_desc.uncapturedErrorCallbackInfo.callback = _gfx_on_error;
  device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  device_desc.deviceLostCallbackInfo.callback = _gfx_on_device_lost;

  WGPURequestDeviceCallbackInfo device_cb =
      WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
  device_cb.mode = WGPUCallbackMode_WaitAnyOnly;
  device_cb.callback = _gfx_on_device;
  device_cb.userdata1 = &_gfx.device;
  _gfx_wait(wgpuAdapterRequestDevice(_gfx.adapter, &device_desc, device_cb));
  if (!_gfx.device) {
    return GFX_ERR_WGPU;
  }

  _gfx.queue = wgpuDeviceGetQueue(_gfx.device);

  GFX_Error err = gfx_uniforms_init();
  if (err) {
    return err;
  }
  gfx_materials_init();
  return GFX_ERR_OK;
}

// Terminate the gfx graphics library
void gfx_graphics_terminate() {
  gfx_materials_terminate();
  gfx_uniforms_terminate();
  if (_gfx.queue)
    wgpuQueueRelease(_gfx.queue);
  if (_gfx.device)
    wgpuDeviceRelease(_gfx.device);
  if (_gfx.adapter)
    wgpuAdapterRelease(_gfx.adapter);
  if (_gfx.instance)
    wgpuInstanceRelease(_gfx.instance);
  memset(&_gfx, 0, sizeof(_gfx));

  glfwTerminate();
}

// Enable the given graphics feature
void gfx_graphics_enable(enum GFX_GraphicsFeature feature) {
  _gfx.features |= 1u << feature;
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
  window->format = caps.formats[0];
  _gfx.surface_format = window->format;
  wgpuSurfaceCapabilitiesFreeMembers(caps);

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

// Begin a render pass on the frame into target, NULL for the window
static void _gfx_pass_begin(const GFX_RenderTexture *target,
                            WGPULoadOp color_load) {
  WGPURenderPassColorAttachment color = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  color.view = target ? target->texture.view : _gfx.view;
  color.loadOp = color_load;
  color.storeOp = WGPUStoreOp_Store;
  color.clearValue = _gfx.clear_color;

  WGPURenderPassDepthStencilAttachment depth =
      WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
  depth.view = target ? target->depth_view : _gfx.depth_view;
  depth.depthLoadOp = WGPULoadOp_Clear;
  depth.depthStoreOp = WGPUStoreOp_Discard;
  depth.depthClearValue = 1.0f;

  WGPURenderPassDescriptor pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  pass_desc.colorAttachmentCount = 1;
  pass_desc.colorAttachments = &color;
  pass_desc.depthStencilAttachment = &depth;
  _gfx.pass = wgpuCommandEncoderBeginRenderPass(_gfx.encoder, &pass_desc);
  _gfx.pass_target = target;
}

static void _gfx_pass_end(void) {
  wgpuRenderPassEncoderEnd(_gfx.pass);
  wgpuRenderPassEncoderRelease(_gfx.pass);
  _gfx.pass = NULL;
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

// Draw the next calls into rect of target with view_projection, NULL target
// for the window
bool gfx_frame_view_begin(GFX_RenderTexture *target, const vec4 rect,
                          mat4 view_projection) {
  if (!_gfx.pass || (target && !target->texture.texture)) {
    return false;
  }
  uint32_t px[4];
  gfx_viewport_pixels(rect, target ? target->texture.width : _gfx.width,
                      target ? target->texture.height : _gfx.height, px);
  if (px[2] == 0 || px[3] == 0) {
    return false;
  }

  // The first window view uses the frame's pass, every other view gets a new
  // one so views don't share depth
  if (target || _gfx.pass_target || _gfx.view_count > 0) {
    _gfx_pass_end();
    WGPULoadOp load = WGPULoadOp_Load;
    if (target && target->cleared_frame != _gfx.frame) {
      target->cleared_frame = _gfx.frame;
      load = WGPULoadOp_Clear;
    }
    _gfx_pass_begin(target, load);
  }
  _gfx.view_count++;

  wgpuRenderPassEncoderSetViewport(_gfx.pass, (float)px[0], (float)px[1],
                                   (float)px[2], (float)px[3], 0.0f, 1.0f);
  wgpuRenderPassEncoderSetScissorRect(_gfx.pass, px[0], px[1], px[2], px[3]);
  gfx_uniforms_view_begin(view_projection);
  return true;
}

// Begin drawing a frame on window
void gfx_frame_begin(GFX_Window *window) {
  _GFX_Window *win = window;

  // Follow window resizes, skip the frame while minimized
  int w, h;
  GLFWCall(glfwGetFramebufferSize(win->glfw, &w, &h));
  if ((w != win->width || h != win->height) && !_gfx_window_configure(win)) {
    return;
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
    return;
  }

  _gfx.texture = surface_texture.texture;
  _gfx.view = wgpuTextureCreateView(_gfx.texture, NULL);
  _gfx.depth_view = win->depth_view;
  _gfx.width = win->width;
  _gfx.height = win->height;
  _gfx.view_count = 0;
  _gfx.frame++;
  _gfx.encoder = wgpuDeviceCreateCommandEncoder(_gfx.device, NULL);

  _gfx_pass_begin(NULL, WGPULoadOp_Clear);
  gfx_uniforms_frame_begin();
}

// Finish the frame and show it on window
void gfx_frame_end(GFX_Window *window) {
  // No frame this tick (minimized or surface lost)
  if (!_gfx.pass) {
    return;
  }

  _gfx_pass_end();

  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(_gfx.encoder, NULL);
  // Queue writes land before the submitted commands run
  gfx_uniforms_frame_end();
  wgpuQueueSubmit(_gfx.queue, 1, &commands);
  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(_gfx.encoder);

#ifndef __EMSCRIPTEN__
  // The browser presents on its own
  wgpuSurfacePresent(((_GFX_Window *)window)->surface);
  wgpuInstanceProcessEvents(_gfx.instance);
#endif

  wgpuTextureViewRelease(_gfx.view);
  wgpuTextureRelease(_gfx.texture);

  _gfx.pass = NULL;
  _gfx.encoder = NULL;
  _gfx.view = NULL;
  _gfx.texture = NULL;
}
