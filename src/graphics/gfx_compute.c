// Compute dispatches, recorded between gfx_compute_begin and gfx_compute_end
// and encoded together in one compute pass

#include "gfx_internal.h"
#include <assert.h>
#include <gfx/compute.h>
#include <gfx/graphics.h>
#include <stb_ds.h>
#include <stdio.h>
#include <string.h>

// A dispatch waiting for gfx_compute_end
struct _GFX_ComputeDispatch {
  const GFX_ComputeShader *shader;
  WGPUBuffer buffers[GFX_COMPUTE_MAX_STORAGE_BUFFERS];
  // Of its uniform in the arena
  uint32_t offset;
  uint32_t groups[3];
};

// A bind group of the session, for one shader and its buffers
struct _GFX_ComputeGroup {
  const GFX_ComputeShader *shader;
  WGPUBuffer buffers[GFX_COMPUTE_MAX_STORAGE_BUFFERS];
  WGPUBindGroup group;
};

static struct {
  bool recording;
  GFX_GraphicsLimits limits;
  // Session scratch (dynamic arrays)
  struct _GFX_ComputeDispatch *dispatches;
  struct _GFX_ComputeGroup *groups;
  // Uniforms of the session, one GFX_COMPUTE_MAX_UNIFORM_SIZE slot each
  uint8_t *uniforms;
  // GPU copy of the uniforms, grows when needed
  WGPUBuffer uniform_buffer;
  uint64_t uniform_capacity;
} _compute;

static uint32_t _gfx_compute_uniform_binding_size(uint32_t uniform_size) {
  // WGSL uniform structs are sized in multiples of 16 bytes
  return (uniform_size + 15) & ~15u;
}

// Workgroups as x * y, x at most limit
void gfx_compute_groups_split(uint64_t groups, uint32_t limit, uint32_t *x,
                              uint32_t *y) {
  if (groups == 0) {
    *x = 0;
    *y = 1;
    return;
  }
  *x = groups < limit ? (uint32_t)groups : limit;
  *y = (uint32_t)((groups + *x - 1) / *x);
}

// Workgroups covering count invocations, split in 2D past the dimension limit
void gfx_compute_groups(uint64_t count, uint32_t workgroup_size,
                        uint32_t *groups_x, uint32_t *groups_y) {
  uint64_t groups = (count + workgroup_size - 1) / workgroup_size;
  gfx_compute_groups_split(groups,
                           gfx_graphics_limits().max_workgroups_per_dimension,
                           groups_x, groups_y);
}

// Start recording dispatches
void gfx_compute_begin(void) {
  assert(!_compute.recording);
  _compute.recording = true;
  _compute.limits = gfx_graphics_limits();
  arrsetlen(_compute.dispatches, 0);
  arrsetlen(_compute.uniforms, 0);
}

// Record a dispatch of shader over buffers with uniform
void gfx_compute_dispatch(const GFX_ComputeShader *shader,
                          const GFX_StorageBuffer *const *buffers,
                          const void *uniform, uint32_t groups_x,
                          uint32_t groups_y, uint32_t groups_z) {
  assert(_compute.recording);
  uint32_t limit = _compute.limits.max_workgroups_per_dimension;
  if (groups_x > limit || groups_y > limit || groups_z > limit) {
    fprintf(stderr, "[e] %s: %u x %u x %u workgroups, the device allows %u\n",
            shader->label, groups_x, groups_y, groups_z, limit);
    return;
  }

  struct _GFX_ComputeDispatch dispatch = {
      .shader = shader,
      .groups = {groups_x, groups_y, groups_z},
  };
  for (uint32_t i = 0; i < shader->storage_count; i++) {
    dispatch.buffers[i] = buffers[i]->buffer;
  }
  if (shader->uniform_size > 0) {
    dispatch.offset = arrlen(_compute.uniforms);
    uint8_t *slot = arraddnptr(_compute.uniforms, GFX_COMPUTE_MAX_UNIFORM_SIZE);
    memcpy(slot, uniform, shader->uniform_size);
  }
  arrput(_compute.dispatches, dispatch);
}

// Upload the session's uniforms, growing the GPU buffer when needed
static void _gfx_compute_uniforms_upload(void) {
  uint64_t size = arrlen(_compute.uniforms);
  if (size == 0) {
    return;
  }
  if (size > _compute.uniform_capacity) {
    if (_compute.uniform_buffer) {
      wgpuBufferRelease(_compute.uniform_buffer);
    }
    uint64_t capacity = 64 * GFX_COMPUTE_MAX_UNIFORM_SIZE;
    while (capacity < size) {
      capacity *= 2;
    }
    WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    desc.size = capacity;
    _compute.uniform_buffer = wgpuDeviceCreateBuffer(_gfx.device, &desc);
    _compute.uniform_capacity = capacity;
  }
  wgpuQueueWriteBuffer(_gfx.queue, _compute.uniform_buffer, 0,
                       _compute.uniforms, size);
}

// Bind group of the dispatch's shader and buffers, created on first use
static WGPUBindGroup
_gfx_compute_group(const struct _GFX_ComputeDispatch *dispatch) {
  const GFX_ComputeShader *shader = dispatch->shader;
  size_t buffers_size = shader->storage_count * sizeof(WGPUBuffer);
  for (ptrdiff_t i = 0; i < arrlen(_compute.groups); i++) {
    if (_compute.groups[i].shader == shader &&
        memcmp(_compute.groups[i].buffers, dispatch->buffers, buffers_size) ==
            0) {
      return _compute.groups[i].group;
    }
  }

  WGPUBindGroupEntry entries[1 + GFX_COMPUTE_MAX_STORAGE_BUFFERS];
  size_t count = 0;
  if (shader->uniform_size > 0) {
    entries[count] = WGPU_BIND_GROUP_ENTRY_INIT;
    entries[count].binding = 0;
    entries[count].buffer = _compute.uniform_buffer;
    entries[count].size =
        _gfx_compute_uniform_binding_size(shader->uniform_size);
    count++;
  }
  for (uint32_t i = 0; i < shader->storage_count; i++) {
    entries[count] = WGPU_BIND_GROUP_ENTRY_INIT;
    entries[count].binding = 1 + i;
    entries[count].buffer = dispatch->buffers[i];
    count++;
  }

  WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  desc.layout = shader->layout;
  desc.entryCount = count;
  desc.entries = entries;
  struct _GFX_ComputeGroup group = {
      .shader = shader,
      .group = wgpuDeviceCreateBindGroup(_gfx.device, &desc),
  };
  memcpy(group.buffers, dispatch->buffers, buffers_size);
  arrput(_compute.groups, group);
  return group.group;
}

// Encode the recorded dispatches in one compute pass and submit them
void gfx_compute_end(void) {
  assert(_compute.recording);
  _compute.recording = false;
  if (arrlen(_compute.dispatches) == 0) {
    return;
  }

  _gfx_compute_uniforms_upload();

  WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(_gfx.device, NULL);
  WGPUComputePassEncoder pass =
      wgpuCommandEncoderBeginComputePass(encoder, NULL);
  for (ptrdiff_t i = 0; i < arrlen(_compute.dispatches); i++) {
    const struct _GFX_ComputeDispatch *dispatch = &_compute.dispatches[i];
    wgpuComputePassEncoderSetPipeline(pass, dispatch->shader->pipeline);
    wgpuComputePassEncoderSetBindGroup(
        pass, 0, _gfx_compute_group(dispatch),
        dispatch->shader->uniform_size > 0 ? 1 : 0, &dispatch->offset);
    wgpuComputePassEncoderDispatchWorkgroups(
        pass, dispatch->groups[0], dispatch->groups[1], dispatch->groups[2]);
  }
  wgpuComputePassEncoderEnd(pass);
  wgpuComputePassEncoderRelease(pass);

  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, NULL);
  wgpuQueueSubmit(_gfx.queue, 1, &commands);
  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(encoder);

  // Bind groups only live for the session
  for (ptrdiff_t i = 0; i < arrlen(_compute.groups); i++) {
    wgpuBindGroupRelease(_compute.groups[i].group);
  }
  arrsetlen(_compute.groups, 0);
}

void gfx_compute_terminate(void) {
  if (_compute.uniform_buffer) {
    wgpuBufferRelease(_compute.uniform_buffer);
  }
  arrfree(_compute.dispatches);
  arrfree(_compute.groups);
  arrfree(_compute.uniforms);
  memset(&_compute, 0, sizeof(_compute));
}
