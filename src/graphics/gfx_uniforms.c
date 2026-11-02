// Engine uniforms: the view-projection of each view and the model of each
// draw, uploaded together once per frame

#include "gfx_internal.h"
#include <gfx/render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dynamic offsets must be multiples of minUniformBufferOffsetAlignment, which
// is at most 256 on every device
#define GFX_UNIFORM_SLOT_SIZE 256
// Matrices per frame (1MB), raise it if games need more
#define GFX_UNIFORM_SLOT_COUNT 4096

static struct {
  WGPUBindGroupLayout frame_layout;
  WGPUBindGroupLayout object_layout;
  WGPUBindGroup frame_group;
  WGPUBuffer object_buffer;
  WGPUBindGroup object_group;

  // Matrices of this frame, waiting for the upload
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

  _uniforms.frame_layout = _gfx_uniform_layout(WGPU_TRUE);
  _uniforms.object_layout = _gfx_uniform_layout(WGPU_TRUE);

  _gfx.pipeline_layout = gfx_uniforms_pipeline_layout(NULL);

  _uniforms.object_buffer = _gfx_uniform_buffer(
      (uint64_t)GFX_UNIFORM_SLOT_COUNT * GFX_UNIFORM_SLOT_SIZE);
  _uniforms.frame_group =
      _gfx_uniform_group(_uniforms.frame_layout, _uniforms.object_buffer);
  _uniforms.object_group =
      _gfx_uniform_group(_uniforms.object_layout, _uniforms.object_buffer);

  return GFX_ERR_OK;
}

// Pipeline layout with an optional material
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
  wgpuPipelineLayoutRelease(_gfx.pipeline_layout);
  wgpuBindGroupLayoutRelease(_uniforms.object_layout);
  wgpuBindGroupLayoutRelease(_uniforms.frame_layout);
  free(_uniforms.object_data);
  memset(&_uniforms, 0, sizeof(_uniforms));
}

// Store a matrix for this frame at offset, false when full
static bool _gfx_uniforms_push(mat4 matrix, uint32_t *offset) {
  if (_uniforms.object_count >= GFX_UNIFORM_SLOT_COUNT) {
    if (_uniforms.object_count++ == GFX_UNIFORM_SLOT_COUNT) {
      fprintf(stderr, "[e] uniform arena full (%d matrices)\n",
              GFX_UNIFORM_SLOT_COUNT);
    }
    return false;
  }
  *offset = _uniforms.object_count * GFX_UNIFORM_SLOT_SIZE;
  memcpy(_uniforms.object_data + *offset, matrix, sizeof(mat4));
  _uniforms.object_count += 1;
  return true;
}

// Bind both groups to the identity matrix
static void _gfx_uniforms_bind_identity(void) {
  uint32_t offset = 0;
  wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 0, _uniforms.frame_group, 1,
                                    &offset);
  wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 1, _uniforms.object_group, 1,
                                    &offset);
}

// Start the frame's matrices, the first one is the identity
void gfx_uniforms_frame_begin(void) {
  _uniforms.object_count = 0;
  mat4 identity = GLM_MAT4_IDENTITY_INIT;
  uint32_t offset;
  _gfx_uniforms_push(identity, &offset);
}

// Prepare uniforms for the new pass
void gfx_uniforms_view_begin(mat4 view_projection) {
  _gfx_uniforms_bind_identity();
  uint32_t offset;
  if (_gfx_uniforms_push(view_projection, &offset)) {
    wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 0, _uniforms.frame_group, 1,
                                      &offset);
  }
}

// Upload the matrices of this frame, before the submit
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

// Set the model matrix used by the next draws
void gfx_draw_set_model(mat4 model) {
  // When full, extra draws keep the last model
  uint32_t offset;
  if (_gfx.pass && _gfx_uniforms_push(model, &offset)) {
    wgpuRenderPassEncoderSetBindGroup(_gfx.pass, 1, _uniforms.object_group, 1,
                                      &offset);
  }
}
