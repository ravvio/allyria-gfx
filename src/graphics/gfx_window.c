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

// Terminate the gfx grpahics library
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
  WGPUTextureDescriptor depth_desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
  depth_desc.usage = WGPUTextureUsage_RenderAttachment;
  depth_desc.dimension = WGPUTextureDimension_2D;
  depth_desc.size = (WGPUExtent3D){w, h, 1};
  depth_desc.format = GFX_DEPTH_FORMAT;
  window->depth = wgpuDeviceCreateTexture(_gfx.device, &depth_desc);
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

// Acquire the next surface texture and begin the frame render pass (cleared)
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
  _gfx.encoder = wgpuDeviceCreateCommandEncoder(_gfx.device, NULL);

  WGPURenderPassColorAttachment color = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  color.view = _gfx.view;
  color.loadOp = WGPULoadOp_Clear;
  color.storeOp = WGPUStoreOp_Store;
  color.clearValue = _gfx.clear_color;

  WGPURenderPassDepthStencilAttachment depth =
      WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
  depth.view = win->depth_view;
  depth.depthLoadOp = WGPULoadOp_Clear;
  depth.depthStoreOp = WGPUStoreOp_Discard;
  depth.depthClearValue = 1.0f;

  WGPURenderPassDescriptor pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  pass_desc.colorAttachmentCount = 1;
  pass_desc.colorAttachments = &color;
  pass_desc.depthStencilAttachment = &depth;
  _gfx.pass = wgpuCommandEncoderBeginRenderPass(_gfx.encoder, &pass_desc);
  gfx_uniforms_frame_begin();
}

// End the frame render pass, submit and present
void gfx_frame_end(GFX_Window *window) {
  // No frame this tick (minimized or surface lost)
  if (!_gfx.pass) {
    return;
  }

  wgpuRenderPassEncoderEnd(_gfx.pass);
  wgpuRenderPassEncoderRelease(_gfx.pass);

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
