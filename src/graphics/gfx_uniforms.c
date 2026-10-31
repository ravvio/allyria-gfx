// Engine uniforms (see SOURCES.md, "Uniforms and bind groups"):
// - group 0, frame: view-projection matrix, written once per frame
// - group 1, object: model matrix per draw, stored in a per-frame arena of
//   256 byte slots, bound with a dynamic offset and uploaded at frame end

#include "../../include/gfx/graphics/gfx_wgpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dynamic offsets must be multiples of minUniformBufferOffsetAlignment, which
// is at most 256 on every device
#define GFX_UNIFORM_SLOT_SIZE 256
// Draws per frame with their own model matrix (1MB), raise it or size it from
// the previous frame if games need more
#define GFX_UNIFORM_SLOT_COUNT 4096

static struct {
  WGPUBindGroupLayout frame_layout;
  WGPUBindGroupLayout object_layout;
  WGPUBuffer frame_buffer;
  WGPUBindGroup frame_group;
  WGPUBuffer object_buffer;
  WGPUBindGroup object_group;

  // CPU copy of the object arena, uploaded at frame end
  uint8_t *object_data;
  // Slots used this frame, can go past the capacity to report overflows once
  uint32_t object_count;
} _uniforms;

static WGPUBindGroupLayout _gfx_uniform_layout(WGPUBool dynamic) {
  WGPUBindGroupLayoutEntry entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
  entry.binding = 0;
  entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
  entry.buffer.type = WGPUBufferBindingType_Uniform;
  entry.buffer.hasDynamicOffset = dynamic;
  entry.buffer.minBindingSize = sizeof(mat4);

  WGPUBindGroupLayoutDescriptor desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  desc.entryCount = 1;
  desc.entries = &entry;
  return wgpuDeviceCreateBindGroupLayout(_gfx.device, &desc);
}

static WGPUBuffer _gfx_uniform_buffer(uint64_t size) {
  WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
  desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  desc.size = size;
  return wgpuDeviceCreateBuffer(_gfx.device, &desc);
}

// Bind group exposing one mat4 of the buffer
static WGPUBindGroup _gfx_uniform_group(WGPUBindGroupLayout layout,
                                        WGPUBuffer buffer) {
  WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
  entry.binding = 0;
  entry.buffer = buffer;
  entry.size = sizeof(mat4);

  WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  desc.layout = layout;
  desc.entryCount = 1;
  desc.entries = &entry;
  return wgpuDeviceCreateBindGroup(_gfx.device, &desc);
}

// Create the shared layouts and uniform buffers
GFX_Error gfx_uniforms_init(void) {
  _uniforms.object_data = calloc(GFX_UNIFORM_SLOT_COUNT, GFX_UNIFORM_SLOT_SIZE);
  if (_uniforms.object_data == NULL) {
    fprintf(stderr, "[e] failed to allocate the uniform arena\n");
    return GFX_ERR_FAILED;
  }

  _uniforms.frame_layout = _gfx_uniform_layout(WGPU_FALSE);
  _uniforms.object_layout = _gfx_uniform_layout(WGPU_TRUE);

  _gfx.pipeline_layout = gfx_uniforms_pipeline_layout(NULL);

  _uniforms.frame_buffer = _gfx_uniform_buffer(sizeof(mat4));
  _uniforms.frame_group =
      _gfx_uniform_group(_uniforms.frame_layout, _uniforms.frame_buffer);
  _uniforms.object_buffer = _gfx_uniform_buffer(
      (uint64_t)GFX_UNIFORM_SLOT_COUNT * GFX_UNIFORM_SLOT_SIZE);
  _uniforms.object_group =
      _gfx_uniform_group(_uniforms.object_layout, _uniforms.object_buffer);

  // Identity until a camera is set
  mat4 identity = GLM_MAT4_IDENTITY_INIT;
  gfx_frame_set_view_projection(identity);

  return GFX_ERR_OK;
}

// Every pipeline layout shares the frame and object group layouts, so their
// bind groups work across pipelines
WGPUPipelineLayout gfx_uniforms_pipeline_layout(WGPUBindGroupLayout material) {
  WGPUBindGroupLayout layouts[] = {
      _uniforms.frame_layout,
      _uniforms.object_layout,
      material,
  };
  WGPUPipelineLayoutDescriptor desc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  desc.bindGroupLayoutCount = material ? 3 : 2;
  desc.bindGroupLayouts = layouts;
  return wgpuDeviceCreatePipelineLayout(_gfx.device, &desc);
}

void gfx_uniforms_terminate(void) {
  if (_uniforms.object_data == NULL) {
    return;
  }
  wgpuBindGroupRelease(_uniforms.object_group);
  wgpuBufferRelease(_uniforms.object_buffer);
  wgpuBindGroupRelease(_uniforms.frame_group);
  wgpuBufferRelease(_uniforms.frame_buffer);
  wgpuPipelineLayoutRelease(_gfx.pipeline_layout);
  wgpuBindGroupLayoutRelease(_uniforms.object_layout);
  wgpuBindGroupLayoutRelease(_uniforms.frame_layout);
  free(_uniforms.object_data);
  memset(&_uniforms, 0, sizeof(_uniforms));
}

// Reset the arena and bind both groups to the new render pass
void gfx_uniforms_frame_begin(void) {
  _uniforms.object_count = 0;

  uint32_t offset = 0;
  wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 0, _uniforms.frame_group, 0,
                                    NULL);
  wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 1, _uniforms.object_group, 1,
                                    &offset);
}

// Upload the model matrices of this frame, before the submit
void gfx_uniforms_frame_end(void) {
  uint32_t count = _uniforms.object_count < GFX_UNIFORM_SLOT_COUNT
                        ? _uniforms.object_count
                        : GFX_UNIFORM_SLOT_COUNT;
  if (count > 0) {
    wgpuQueueWriteBuffer(_gfx.queue, _uniforms.object_buffer, 0,
                         _uniforms.object_data,
                         (size_t)count * GFX_UNIFORM_SLOT_SIZE);
  }
}

// Set the camera projection * view for the next frame
void gfx_frame_set_view_projection(mat4 view_projection) {
  wgpuQueueWriteBuffer(_gfx.queue, _uniforms.frame_buffer, 0, view_projection,
                       sizeof(mat4));
}

// Set the model matrix used by the next draws
void gfx_draw_set_model(mat4 model) {
  if (!_gfx.pass) {
    return;
  }

  if (_uniforms.object_count >= GFX_UNIFORM_SLOT_COUNT) {
    // Full, extra draws keep the last bound model
    if (_uniforms.object_count++ == GFX_UNIFORM_SLOT_COUNT) {
      fprintf(stderr, "[e] uniform arena full (%d draws), see gfx_uniforms.c\n",
              GFX_UNIFORM_SLOT_COUNT);
    }
    return;
  }

  uint32_t offset = _uniforms.object_count * GFX_UNIFORM_SLOT_SIZE;
  memcpy(_uniforms.object_data + offset, model, sizeof(mat4));
  _uniforms.object_count += 1;

  wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 1, _uniforms.object_group, 1,
                                    &offset);
}
