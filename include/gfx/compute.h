#ifndef GFX_COMPUTE_DECL
#define GFX_COMPUTE_DECL

#include <gfx/core.h>
#include <gfx/render.h>
#include <webgpu/webgpu.h>

// Storage Buffer

typedef struct {
  WGPUBuffer buffer;
  uint64_t size; // bytes
} GFX_StorageBuffer;

// Create a storage buffer, vertex: also drawable as vertex buffer
GFX_StorageBuffer gfx_storage_buffer_create(uint64_t size, bool vertex);
// Write size bytes of data at offset
void gfx_storage_buffer_write(const GFX_StorageBuffer *buffer, uint64_t offset,
                              const void *data, uint64_t size);
// Read size bytes at offset into out, blocks until the GPU is done with it
GFX_Error gfx_storage_buffer_read(const GFX_StorageBuffer *buffer,
                                  uint64_t offset, void *out, uint64_t size);
// Use the storage buffer as a vertex buffer
GFX_VertexBuffer gfx_storage_buffer_as_vertex(const GFX_StorageBuffer *buffer);
void gfx_storage_buffer_destroy(const GFX_StorageBuffer *buffer);

// Compute Shader, WGSL with entry point comp_main. Its bindings are in
// @group(0), declared with uniform_size and storage_count:
//   @binding(0)   uniform of uniform_size bytes (only if uniform_size > 0)
//   @binding(1+i) storage buffer i, var<storage, read_write>

// A uniform is one slot of 256 bytes, the dynamic offset alignment
#define GFX_COMPUTE_MAX_UNIFORM_SIZE 256
// WebGPU's default maxStorageBuffersPerShaderStage
#define GFX_COMPUTE_MAX_STORAGE_BUFFERS 8

typedef struct {
  // Shown in error messages, the file path for loaded shaders
  const char *label;
  WGPUShaderModule module;
  uint32_t uniform_size;
  uint32_t storage_count;
  WGPUBindGroupLayout layout;
  WGPUComputePipeline pipeline;
} GFX_ComputeShader;

// Load a WGSL file
GFX_ComputeShader gfx_compute_shader_create(const char *filepath,
                                            uint32_t uniform_size,
                                            uint32_t storage_count);
// WGSL source in memory (null terminated), label must outlive the shader
GFX_ComputeShader gfx_compute_shader_create_wgsl(const char *label,
                                                 const char *source,
                                                 uint32_t uniform_size,
                                                 uint32_t storage_count);
void gfx_compute_shader_destroy(const GFX_ComputeShader *shader);

// Dispatch

// Workgroups covering count invocations, split in 2D past the dimension limit.
// The shader rebuilds its index as (id.y * groups_x + id.x) * workgroup_size
// + local index, and skips indices past count.
void gfx_compute_groups(uint64_t count, uint32_t workgroup_size,
                        uint32_t *groups_x, uint32_t *groups_y);

// Record dispatches, then submit them together in gfx_compute_end. Shaders
// and buffers must stay alive until then.
void gfx_compute_begin(void);
// buffers has the shader's storage_count entries, uniform its uniform_size
// bytes (NULL without uniform)
void gfx_compute_dispatch(const GFX_ComputeShader *shader,
                          const GFX_StorageBuffer *const *buffers,
                          const void *uniform, uint32_t groups_x,
                          uint32_t groups_y, uint32_t groups_z);
void gfx_compute_end(void);

#endif
