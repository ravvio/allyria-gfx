// Methods to create and draw WebGPU vertex, index and storage buffers

#include "gfx_internal.h"
#include <assert.h>
#include <gfx/compute.h>
#include <gfx/graphics.h>
#include <gfx/render.h>
#include <stdio.h>
#include <string.h>

// Create a GPU buffer and upload data to it if not NULL
static WGPUBuffer _gfx_buffer_create(WGPUBufferUsage usage, uint64_t size,
                                     const void *data) {
  // Queue writes need 4 byte aligned sizes
  assert(size % 4 == 0);

  WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
  desc.usage = usage | WGPUBufferUsage_CopyDst;
  desc.size = size;
  WGPUBuffer buffer = wgpuDeviceCreateBuffer(_gfx.device, &desc);
  if (data) {
    wgpuQueueWriteBuffer(_gfx.queue, buffer, 0, data, size);
  }
  return buffer;
}

// ============================================================================
// Storage Buffer
// ============================================================================

// Create a storage buffer of size bytes
GFX_StorageBuffer gfx_storage_buffer_create(uint64_t size, bool vertex) {
  GFX_StorageBuffer sb = {0};
  uint64_t max = gfx_graphics_limits().max_buffer_size;
  if (size > max) {
    fprintf(stderr,
            "[e] storage buffer of %llu bytes, the device allows %llu\n",
            (unsigned long long)size, (unsigned long long)max);
    return sb;
  }

  // CopySrc for readback
  WGPUBufferUsage usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
  if (vertex) {
    usage |= WGPUBufferUsage_Vertex;
  }
  sb.buffer = _gfx_buffer_create(usage, size, NULL);
  sb.size = size;
  return sb;
}

// Write size bytes of data at offset
void gfx_storage_buffer_write(const GFX_StorageBuffer *buffer, uint64_t offset,
                              const void *data, uint64_t size) {
  assert(offset % 4 == 0 && size % 4 == 0);
  assert(offset + size <= buffer->size);
  wgpuQueueWriteBuffer(_gfx.queue, buffer->buffer, offset, data, size);
}

static void _gfx_on_buffer_mapped(WGPUMapAsyncStatus status,
                                  WGPUStringView message, void *userdata1,
                                  void *userdata2) {
  (void)userdata2;
  if (status != WGPUMapAsyncStatus_Success) {
    fprintf(stderr, "[e] buffer readback failed\n| %.*s\n",
            (int)(message.length == WGPU_STRLEN ? strlen(message.data)
                                                : message.length),
            message.data);
  }
  *(bool *)userdata1 = status == WGPUMapAsyncStatus_Success;
}

// Read size bytes at offset into out
GFX_Error gfx_storage_buffer_read(const GFX_StorageBuffer *buffer,
                                  uint64_t offset, void *out, uint64_t size) {
  // Buffer copies need 4 byte aligned offsets and sizes
  assert(offset % 4 == 0 && size % 4 == 0);
  assert(offset + size <= buffer->size);

  // MapRead can't be combined with Storage, so copy into a staging buffer
  WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
  desc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
  desc.size = size;
  WGPUBuffer staging = wgpuDeviceCreateBuffer(_gfx.device, &desc);

  WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(_gfx.device, NULL);
  wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer->buffer, offset, staging,
                                       0, size);
  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, NULL);
  wgpuQueueSubmit(_gfx.queue, 1, &commands);
  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(encoder);

  // Waits for the copy, and everything submitted before it
  bool mapped = false;
  WGPUBufferMapCallbackInfo callback = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
  callback.mode = WGPUCallbackMode_WaitAnyOnly;
  callback.callback = _gfx_on_buffer_mapped;
  callback.userdata1 = &mapped;
  gfx_wait(wgpuBufferMapAsync(staging, WGPUMapMode_Read, 0, size, callback));

  if (mapped) {
    memcpy(out, wgpuBufferGetConstMappedRange(staging, 0, size), size);
    wgpuBufferUnmap(staging);
  }
  wgpuBufferRelease(staging);
  return mapped ? GFX_ERR_OK : GFX_ERR_WGPU;
}

// The same buffer as vertices, described by the drawing pipeline's layout
GFX_VertexBuffer gfx_storage_buffer_as_vertex(const GFX_StorageBuffer *buffer) {
  assert(wgpuBufferGetUsage(buffer->buffer) & WGPUBufferUsage_Vertex);
  return (GFX_VertexBuffer){.buffer = buffer->buffer};
}

void gfx_storage_buffer_destroy(const GFX_StorageBuffer *buffer) {
  if (buffer->buffer) {
    wgpuBufferRelease(buffer->buffer);
  }
}

// ============================================================================
// Vertex Buffer
// ============================================================================

// Create a vertex buffer
GFX_VertexBuffer gfx_vertex_buffer_create(uint32_t size, const void *data) {
  GFX_VertexBuffer vb = {
      .buffer = _gfx_buffer_create(WGPUBufferUsage_Vertex, size, data),
  };
  return vb;
}

// Destroy a vertex buffer
void gfx_vertex_buffer_destroy(const GFX_VertexBuffer *vb) {
  wgpuBufferRelease(vb->buffer);
}

// Overwrite the start of a vertex buffer
void gfx_vertex_buffer_write(const GFX_VertexBuffer *vb, const void *data,
                             uint32_t size) {
  assert(size % 4 == 0);
  wgpuQueueWriteBuffer(_gfx.queue, vb->buffer, 0, data, size);
}

// Bind a vertex buffer to the current frame
void gfx_vertex_buffer_bind(const GFX_VertexBuffer *vb) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderSetVertexBuffer(_gfx.pass, 0, vb->buffer, 0,
                                         WGPU_WHOLE_SIZE);
  }
}

// ============================================================================
// Index Buffer
// ============================================================================

// Create an index buffer
GFX_IndexBuffer gfx_index_buffer_create(uint32_t count, const uint32_t *data) {
  GFX_IndexBuffer ib = {
      .buffer = _gfx_buffer_create(WGPUBufferUsage_Index,
                                   count * sizeof(uint32_t), data),
      .count = count,
  };
  return ib;
}

// Destroy an index buffer
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib) {
  wgpuBufferRelease(ib->buffer);
}

// Bind an index buffer to the current frame
void gfx_index_buffer_bind(const GFX_IndexBuffer *ib) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderSetIndexBuffer(
        _gfx.pass, ib->buffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  }
}

// Get count of indices inside buffer
uint32_t gfx_index_buffer_get_count(const GFX_IndexBuffer *ib) {
  return ib->count;
}

// ============================================================================
// Vertex Layout
// ============================================================================

// Add an attribute of `count` f32 to the layout
void gfx_vertex_layout_push_f32(GFX_VertexLayout *layout, uint32_t count) {
  static const WGPUVertexFormat formats[] = {
      WGPUVertexFormat_Float32,
      WGPUVertexFormat_Float32x2,
      WGPUVertexFormat_Float32x3,
      WGPUVertexFormat_Float32x4,
  };
  assert(count >= 1 && count <= 4);
  assert(layout->count < GFX_VERTEX_LAYOUT_MAX_ATTRIBUTES);

  layout->attributes[layout->count] = (WGPUVertexAttribute){
      .format = formats[count - 1],
      .offset = layout->stride,
      .shaderLocation = layout->count,
  };
  layout->count += 1;
  layout->stride += count * sizeof(float);
}

// ============================================================================
// Draw
// ============================================================================

// Draw `count` indices of the bound index buffer
void gfx_draw_indexed(uint32_t count) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderDrawIndexed(_gfx.pass, count, 1, 0, 0, 0);
  }
}

// Draw `instance_count` instances of `vertex_count` vertices, without index
// buffer
void gfx_draw_instanced(uint32_t vertex_count, uint32_t instance_count,
                        uint32_t first_instance) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderDraw(_gfx.pass, vertex_count, instance_count, 0,
                              first_instance);
  }
}
