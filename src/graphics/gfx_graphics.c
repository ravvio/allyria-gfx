#include "gfx_internal.h"
#include <gfx/graphics.h>
#include <string.h>

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
void gfx_wait(WGPUFuture future) {
  WGPUFutureWaitInfo info = {.future = future};
  wgpuInstanceWaitAny(_gfx.instance, 1, &info, UINT64_MAX);
}

// *** LIMITS ***

static GFX_GraphicsLimits _gfx_limits_from(const WGPULimits *limits) {
  return (GFX_GraphicsLimits){
      .max_buffer_size = limits->maxBufferSize,
      .max_storage_binding_size = limits->maxStorageBufferBindingSize,
      .max_workgroups_per_dimension = limits->maxComputeWorkgroupsPerDimension,
      .max_workgroup_invocations = limits->maxComputeInvocationsPerWorkgroup,
  };
}

// Requested limits for the device, false when the adapter can't provide one
static bool _gfx_required_limits(WGPULimits *required) {
  GFX_GraphicsLimits want = _gfx.required_limits;
  GFX_GraphicsLimits have = gfx_graphics_adapter_limits();
  bool ok = true;

#define _GFX_REQUIRE(field, wgpu_field)                                        \
  if (want.field) {                                                            \
    if (want.field > have.field) {                                             \
      fprintf(stderr, "[e] %s of %llu required, the adapter allows %llu\n",    \
              #field, (unsigned long long)want.field,                          \
              (unsigned long long)have.field);                                 \
      ok = false;                                                              \
    }                                                                          \
    required->wgpu_field = want.field;                                         \
  }
  _GFX_REQUIRE(max_buffer_size, maxBufferSize);
  _GFX_REQUIRE(max_storage_binding_size, maxStorageBufferBindingSize);
  _GFX_REQUIRE(max_workgroups_per_dimension, maxComputeWorkgroupsPerDimension);
  _GFX_REQUIRE(max_workgroup_invocations, maxComputeInvocationsPerWorkgroup);
#undef _GFX_REQUIRE

  return ok;
}

// Ask for higher limits than the defaults, before gfx_graphics_init
void gfx_graphics_require_limits(const GFX_GraphicsLimits *limits) {
  if (_gfx.device) {
    fprintf(stderr, "[e] limits must be required before gfx_graphics_init\n");
    return;
  }
  _gfx.required_limits = *limits;
}

// *** GRAPHICS ***

// Create the instance and request the adapter, once
static GFX_Error _gfx_adapter_request(void) {
  if (_gfx.adapter) {
    return GFX_ERR_OK;
  }

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
  gfx_wait(wgpuInstanceRequestAdapter(_gfx.instance, NULL, adapter_cb));
  if (!_gfx.adapter) {
    return GFX_ERR_WGPU;
  }

  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
  wgpuAdapterGetInfo(_gfx.adapter, &info);
  fprintf(stderr, "[i] status: Using WebGPU adapter %.*s\n",
          (int)info.device.length, info.device.data);
  wgpuAdapterInfoFreeMembers(info);
  return GFX_ERR_OK;
}

// Initialize the gfx graphics library
GFX_Error gfx_graphics_init() {
  // Initialize GLFW
  if (glfwInit() == GLFW_FALSE) {
    gfx_glfw_errors_check();
    return GFX_ERR_GLFW;
  }
  fprintf(stderr, "[i] status: Using GLFW %s\n", glfwGetVersionString());

  GFX_Error err = _gfx_adapter_request();
  if (err) {
    return err;
  }

  // Defaults, except what was required
  WGPULimits required_limits = WGPU_LIMITS_INIT;
  if (!_gfx_required_limits(&required_limits)) {
    return GFX_ERR_WGPU;
  }

  // Request the device
  WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
  device_desc.requiredLimits = &required_limits;
  device_desc.uncapturedErrorCallbackInfo.callback = _gfx_on_error;
  device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  device_desc.deviceLostCallbackInfo.callback = _gfx_on_device_lost;

  WGPURequestDeviceCallbackInfo device_cb =
      WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
  device_cb.mode = WGPUCallbackMode_WaitAnyOnly;
  device_cb.callback = _gfx_on_device;
  device_cb.userdata1 = &_gfx.device;
  gfx_wait(wgpuAdapterRequestDevice(_gfx.adapter, &device_desc, device_cb));
  if (!_gfx.device) {
    return GFX_ERR_WGPU;
  }

  _gfx.queue = wgpuDeviceGetQueue(_gfx.device);

  GFX_GraphicsLimits limits = gfx_graphics_limits();
  fprintf(stderr,
          "[i] status: Max buffer %llu MiB, storage binding %llu MiB, %u "
          "workgroups per dimension\n",
          (unsigned long long)(limits.max_buffer_size >> 20),
          (unsigned long long)(limits.max_storage_binding_size >> 20),
          limits.max_workgroups_per_dimension);

  err = gfx_uniforms_init();
  if (err) {
    return err;
  }
  gfx_materials_init();
  return GFX_ERR_OK;
}

// Terminate the gfx graphics library
void gfx_graphics_terminate() {
  gfx_compute_terminate();
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

// Limits of the device
GFX_GraphicsLimits gfx_graphics_limits(void) {
  WGPULimits limits = WGPU_LIMITS_INIT;
  wgpuDeviceGetLimits(_gfx.device, &limits);
  return _gfx_limits_from(&limits);
}

// The most the adapter could give, also before gfx_graphics_init
GFX_GraphicsLimits gfx_graphics_adapter_limits(void) {
  WGPULimits limits = WGPU_LIMITS_INIT;
  if (_gfx_adapter_request() != GFX_ERR_OK ||
      wgpuAdapterGetLimits(_gfx.adapter, &limits) != WGPUStatus_Success) {
    return (GFX_GraphicsLimits){0};
  }
  return _gfx_limits_from(&limits);
}
