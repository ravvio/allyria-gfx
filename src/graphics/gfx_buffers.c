// Methods to create and draw WebGPU vertex and index buffers

#include "../../include/gfx/graphics/gfx_wgpu.h"
#include <assert.h>

// Create a GPU buffer and upload data to it if not NULL
static WGPUBuffer _gfx_buffer_create(WGPUBufferUsage usage, uint32_t size,
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

// Destroy a index buffer
void gfx_index_buffer_destroy(const GFX_IndexBuffer *ib) {
  wgpuBufferRelease(ib->buffer);
}

// Bind a index buffer to the current frame
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
