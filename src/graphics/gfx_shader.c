#include "gfx_internal.h"
#include <gfx/graphics.h>
#include <gfx/render.h>
#include <stdio.h>
#include <stdlib.h>

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

// Create a shader module from WGSL code, errors are logged by the device
static GFX_Shader _gfx_shader_create(const char *label, WGPUStringView code) {
  WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgsl.code = code;

  WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  desc.nextInChain = &wgsl.chain;
  desc.label = _gfx_str(label);

  GFX_Shader shader = {
      .label = label,
      .module = wgpuDeviceCreateShaderModule(_gfx.device, &desc),
  };
  return shader;
}

// Create a shader from a WGSL file
GFX_Shader gfx_shader_create(const char *filepath) {
  size_t size;
  char *source = _gfx_file_read(filepath, &size);
  if (source == NULL) {
    GFX_Shader shader = {.label = filepath};
    return shader;
  }

  GFX_Shader shader =
      _gfx_shader_create(filepath, (WGPUStringView){source, size});
  free(source);

  fprintf(stderr, "[i] loaded shader %s\n", filepath);
  return shader;
}

// Create a shader from WGSL source in memory
GFX_Shader gfx_shader_create_wgsl(const char *label, const char *source) {
  return _gfx_shader_create(label, _gfx_str(source));
}

void gfx_shader_destroy(const GFX_Shader *shader) {
  if (shader->module) {
    wgpuShaderModuleRelease(shader->module);
  }
}

// ============================================================================
// Pipeline
// ============================================================================

static int _gfx_feature_enabled(enum GFX_GraphicsFeature feature) {
  return (_gfx.features & (1u << feature)) != 0;
}

// Create a render pipeline drawing triangles to the window surface
GFX_Pipeline gfx_pipeline_create(const GFX_Shader *shader,
                                 const GFX_VertexLayout *layout,
                                 const GFX_MaterialLayout *material) {
  WGPUVertexBufferLayout buffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
  buffer.stepMode = layout->step ? layout->step : WGPUVertexStepMode_Vertex;
  buffer.arrayStride = layout->stride;
  buffer.attributeCount = layout->count;
  buffer.attributes = layout->attributes;

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

  WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
  fragment.module = shader->module;
  fragment.entryPoint = _gfx_str("fs_main");
  fragment.targetCount = 1;
  fragment.targets = &target;

  WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  desc.label = _gfx_str(shader->label);
  desc.layout = material ? material->pipeline_layout : _gfx.pipeline_layout;
  desc.vertex.module = shader->module;
  desc.vertex.entryPoint = _gfx_str("vs_main");
  desc.vertex.bufferCount = 1;
  desc.vertex.buffers = &buffer;
  desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  desc.primitive.frontFace = WGPUFrontFace_CCW;
  if (_gfx_feature_enabled(GFX_GRAPHICS_FEATURE_CULL_BACK)) {
    desc.primitive.cullMode = WGPUCullMode_Back;
  }

  // Every frame has a depth attachment, so every pipeline declares it
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
  desc.fragment = &fragment;

  GFX_Pipeline pipeline = {
      .pipeline = wgpuDeviceCreateRenderPipeline(_gfx.device, &desc),
  };
  return pipeline;
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
