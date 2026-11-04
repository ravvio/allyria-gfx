#include "gfx_internal.h"
#include <assert.h>
#include <gfx/compute.h>
#include <gfx/graphics.h>
#include <gfx/render.h>
#include <stdio.h>
#include <stdlib.h>
#include <webgpu/webgpu.h>

// Read a whole file, the caller frees the result
static char *_gfx_file_read(const char *filepath, size_t *size) {
  FILE *fd = fopen(filepath, "rb");
  if (fd == NULL) {
    fprintf(stderr, "[e] could not open file %s\n", filepath);
    return NULL;
  }

  // Get filesize
  fseek(fd, 0, SEEK_END);
  long filesize = ftell(fd);
  rewind(fd);

  char *buf = filesize > 0 ? malloc(filesize) : NULL;
  if (buf && fread(buf, 1, filesize, fd) != (size_t)filesize) {
    free(buf);
    buf = NULL;
  }
  fclose(fd);

  if (buf == NULL) {
    fprintf(stderr, "[e] could not read file %s\n", filepath);
  }
  *size = filesize;
  return buf;
}

static WGPUStringView _gfx_str(const char *str) {
  return (WGPUStringView){str, WGPU_STRLEN};
}

// ============================================================================
// Shader
// ============================================================================

// Shader module from WGSL code, errors are logged by the device
static WGPUShaderModule _gfx_module_create(const char *label,
                                           WGPUStringView code) {
  WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgsl.code = code;

  WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  desc.nextInChain = &wgsl.chain;
  desc.label = _gfx_str(label);
  return wgpuDeviceCreateShaderModule(_gfx.device, &desc);
}

static GFX_Shader _gfx_shader_create(const char *label, WGPUStringView code,
                                     uint32_t uniform_size,
                                     uint32_t texture_count) {
  GFX_Shader shader = {
      .label = label,
      .module = _gfx_module_create(label, code),
      .uniform_size = uniform_size,
      .texture_count = texture_count,
  };
  if (uniform_size > 0 || texture_count > 0) {
    shader.material_layout =
        gfx_material_layout_create(uniform_size, texture_count);
    shader.pipeline_layout =
        gfx_uniforms_pipeline_layout(shader.material_layout);
  }
  return shader;
}

// Create a shader from a WGSL file
GFX_Shader gfx_shader_create(const char *filepath, uint32_t uniform_size,
                             uint32_t texture_count) {
  size_t size;
  char *source = _gfx_file_read(filepath, &size);
  if (source == NULL) {
    GFX_Shader shader = {.label = filepath};
    return shader;
  }

  GFX_Shader shader = _gfx_shader_create(
      filepath, (WGPUStringView){source, size}, uniform_size, texture_count);
  free(source);

  fprintf(stderr, "[i] loaded shader %s\n", filepath);
  return shader;
}

// Create a shader from WGSL source in memory
GFX_Shader gfx_shader_create_wgsl(const char *label, const char *source,
                                  uint32_t uniform_size,
                                  uint32_t texture_count) {
  return _gfx_shader_create(label, _gfx_str(source), uniform_size,
                            texture_count);
}

void gfx_shader_destroy(const GFX_Shader *shader) {
  if (shader->module) {
    wgpuShaderModuleRelease(shader->module);
  }
  if (shader->material_layout) {
    wgpuPipelineLayoutRelease(shader->pipeline_layout);
    wgpuBindGroupLayoutRelease(shader->material_layout);
  }
}

// ============================================================================
// Pipeline
// ============================================================================

static int _gfx_feature_enabled(enum GFX_GraphicsFeature feature) {
  return (_gfx.features & (1u << feature)) != 0;
}

static GFX_Pipeline _gfx_pipeline_create(
    const char *label, WGPUPipelineLayout pipeline_layout,
    WGPUShaderModule vertex_module, WGPUShaderModule fragment_module,
    const GFX_VertexLayout **buffer_layouts, uint32_t buffer_count) {
  assert(buffer_count <= GFX_MAX_VERTEX_BUFFERS);

  // Target
  WGPUBlendState blend = {
      .color =
          {
              WGPUBlendOperation_Add,
              WGPUBlendFactor_SrcAlpha,
              WGPUBlendFactor_OneMinusSrcAlpha,
          },
      .alpha =
          {
              WGPUBlendOperation_Add,
              WGPUBlendFactor_One,
              WGPUBlendFactor_OneMinusSrcAlpha,
          },
  };
  WGPUColorTargetState target = WGPU_COLOR_TARGET_STATE_INIT;
  target.format = _gfx.surface_format;
  if (_gfx_feature_enabled(GFX_GRAPHICS_FEATURE_ALPHA_BLEND)) {
    target.blend = &blend;
  }

  // Pipeline descriptor
  WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  desc.label = _gfx_str(label);
  desc.layout = pipeline_layout ? pipeline_layout : _gfx.pipeline_layout;

  desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  desc.primitive.frontFace = WGPUFrontFace_CCW;
  if (_gfx_feature_enabled(GFX_GRAPHICS_FEATURE_CULL_BACK)) {
    desc.primitive.cullMode = WGPUCullMode_Back;
  }

  // Depth
  WGPUDepthStencilState depth = WGPU_DEPTH_STENCIL_STATE_INIT;
  depth.format = GFX_DEPTH_FORMAT;
  if (_gfx_feature_enabled(GFX_GRAPHICS_FEATURE_DEPTH_TESTING)) {
    depth.depthWriteEnabled = WGPUOptionalBool_True;
    depth.depthCompare = WGPUCompareFunction_Less;
  } else {
    depth.depthWriteEnabled = WGPUOptionalBool_False;
    depth.depthCompare = WGPUCompareFunction_Always;
  }
  desc.depthStencil = &depth;

  // Buffers and Vertex
  WGPUVertexBufferLayout buffers[GFX_MAX_VERTEX_BUFFERS];
  for (uint32_t i = 0; i < buffer_count; ++i) {
    buffers[i] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    buffers[i].stepMode = buffer_layouts[i]->step ? buffer_layouts[i]->step
                                                  : WGPUVertexStepMode_Vertex;
    buffers[i].arrayStride = buffer_layouts[i]->stride;
    buffers[i].attributeCount = buffer_layouts[i]->count;
    buffers[i].attributes = buffer_layouts[i]->attributes;
  }

  WGPUVertexState vertex = WGPU_VERTEX_STATE_INIT;
  vertex.module = vertex_module;
  vertex.entryPoint = _gfx_str("vert_main");
  vertex.bufferCount = buffer_count;
  vertex.buffers = buffers;

  // Fragment
  WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
  fragment.module = fragment_module;
  fragment.entryPoint = _gfx_str("frag_main");
  fragment.targetCount = 1;
  fragment.targets = &target;

  desc.fragment = &fragment;

  desc.vertex = vertex;

  // Finalize pipeline
  GFX_Pipeline pipeline = {
      .pipeline = wgpuDeviceCreateRenderPipeline(_gfx.device, &desc),
  };
  return pipeline;
}

// Create a render pipeline drawing triangles to the window surface
GFX_Pipeline gfx_pipeline_create(const GFX_Shader *shader,
                                 const GFX_VertexLayout *layout) {
  return _gfx_pipeline_create(shader->label, shader->pipeline_layout,
                              shader->module, shader->module, &layout, 1);
}

// Create a render pipeline drawing a single triangle covering the surface
GFX_Pipeline gfx_pipeline_fullscreen_create(const GFX_Shader *shader) {
  return _gfx_pipeline_create(shader->label, shader->pipeline_layout,
                              gfx_shader_fullscreen()->module, shader->module,
                              NULL, 0);
}

void gfx_pipeline_destroy(const GFX_Pipeline *pipeline) {
  wgpuRenderPipelineRelease(pipeline->pipeline);
}

// Bind a pipeline to the current frame
void gfx_pipeline_bind(const GFX_Pipeline *pipeline) {
  if (_gfx.pass) {
    wgpuRenderPassEncoderSetPipeline(_gfx.pass, pipeline->pipeline);
  }
}

// ============================================================================
// Compute Shader
// ============================================================================

// Bindings of @group(0): binding 0 the uniform, storage buffer i at 1+i
static WGPUBindGroupLayout _gfx_compute_layout_create(uint32_t uniform_size,
                                                      uint32_t storage_count) {
  WGPUBindGroupLayoutEntry entries[1 + GFX_COMPUTE_MAX_STORAGE_BUFFERS];
  size_t count = 0;

  if (uniform_size > 0) {
    entries[count] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entries[count].binding = 0;
    entries[count].visibility = WGPUShaderStage_Compute;
    entries[count].buffer.type = WGPUBufferBindingType_Uniform;
    // One slot of the uniform arena per dispatch
    entries[count].buffer.hasDynamicOffset = WGPU_TRUE;
    // WGSL uniform structs are sized in multiples of 16 bytes
    entries[count].buffer.minBindingSize = (uniform_size + 15) & ~15u;
    count++;
  }

  for (uint32_t i = 0; i < storage_count; i++) {
    entries[count] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entries[count].binding = 1 + i;
    entries[count].visibility = WGPUShaderStage_Compute;
    entries[count].buffer.type = WGPUBufferBindingType_Storage;
    count++;
  }

  WGPUBindGroupLayoutDescriptor desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  desc.entryCount = count;
  desc.entries = entries;
  return wgpuDeviceCreateBindGroupLayout(_gfx.device, &desc);
}

static GFX_ComputeShader _gfx_compute_shader_create(const char *label,
                                                    WGPUStringView code,
                                                    uint32_t uniform_size,
                                                    uint32_t storage_count) {
  assert(uniform_size % 4 == 0 && uniform_size <= GFX_COMPUTE_MAX_UNIFORM_SIZE);
  assert(storage_count <= GFX_COMPUTE_MAX_STORAGE_BUFFERS);

  GFX_ComputeShader shader = {
      .label = label,
      .module = _gfx_module_create(label, code),
      .uniform_size = uniform_size,
      .storage_count = storage_count,
      .layout = _gfx_compute_layout_create(uniform_size, storage_count),
  };

  WGPUPipelineLayoutDescriptor layout_desc =
      WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = &shader.layout;
  WGPUPipelineLayout layout =
      wgpuDeviceCreatePipelineLayout(_gfx.device, &layout_desc);

  WGPUComputePipelineDescriptor desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
  desc.label = _gfx_str(label);
  desc.layout = layout;
  desc.compute.module = shader.module;
  desc.compute.entryPoint = _gfx_str("comp_main");
  shader.pipeline = wgpuDeviceCreateComputePipeline(_gfx.device, &desc);

  // The pipeline keeps what it needs from the layout
  wgpuPipelineLayoutRelease(layout);
  return shader;
}

// Create a compute shader from a WGSL file
GFX_ComputeShader gfx_compute_shader_create(const char *filepath,
                                            uint32_t uniform_size,
                                            uint32_t storage_count) {
  size_t size;
  char *source = _gfx_file_read(filepath, &size);
  if (source == NULL) {
    GFX_ComputeShader shader = {.label = filepath};
    return shader;
  }

  GFX_ComputeShader shader = _gfx_compute_shader_create(
      filepath, (WGPUStringView){source, size}, uniform_size, storage_count);
  free(source);

  fprintf(stderr, "[i] loaded compute shader %s\n", filepath);
  return shader;
}

// Create a compute shader from WGSL source in memory
GFX_ComputeShader gfx_compute_shader_create_wgsl(const char *label,
                                                 const char *source,
                                                 uint32_t uniform_size,
                                                 uint32_t storage_count) {
  return _gfx_compute_shader_create(label, _gfx_str(source), uniform_size,
                                    storage_count);
}

void gfx_compute_shader_destroy(const GFX_ComputeShader *shader) {
  if (shader->pipeline) {
    wgpuComputePipelineRelease(shader->pipeline);
  }
  if (shader->layout) {
    wgpuBindGroupLayoutRelease(shader->layout);
  }
  if (shader->module) {
    wgpuShaderModuleRelease(shader->module);
  }
}
