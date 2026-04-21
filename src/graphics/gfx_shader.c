#include "../../include/gfx/graphics/gfx_gl.h"

u_int32_t gfx_shader_compile(u_int32_t type, const char *source,
                             const int source_lenght) {
  GLCall(u_int32_t id = glCreateShader(type));
  GLCall(glShaderSource(id, 1, &source, &source_lenght));
  GLCall(glCompileShader(id));

  // Error handling
  int result;
  GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
  if (result == GL_FALSE) {
    int length;
    GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
    char *message = (char *)alloca(length * sizeof(char));
    GLCall(glGetShaderInfoLog(id, length, &length, message));
    fprintf(stderr, "[e] failed to compile %s shader\n| %s\n",
            (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", message);
    fprintf(stderr, "| source: %s\n", source);
    GLCall(glDeleteShader(id));
    return 0;
  }
  fprintf(stderr, "[i] shader compiled\n");
  return id;
}

u_int32_t gfx_shader_load(u_int32_t type, const char *filepath) {
  FILE *fd;
  fd = fopen(filepath, "r");
  if (fd == NULL) {
    fprintf(stderr, "[e] could not open file %s\n", filepath);
    return 0;
  }

  // Get filesize
  fseek(fd, 0, SEEK_END);
  u_int64_t filesize = ftell(fd);
  rewind(fd);

  char *buf = malloc(filesize);
  if (buf == NULL) {
    fclose(fd);
    return 0;
  }

  size_t read = fread(buf, sizeof(char), filesize, fd);
  if (read != filesize) {
    free(buf);
    fclose(fd);
    return 0;
  }

  fprintf(stderr, "[i] loaded shader source %s\n", filepath);

  u_int32_t program = gfx_shader_compile(type, buf, filesize);
  free(buf);
  fclose(fd);
  return program;
}

// Create and comile a shader
GFX_Shader gfx_shader_create(const char *vert_filepath,
                             const char *frag_filepath) {
  GLCall(u_int32_t id = glCreateProgram());

  // Compile
  u_int32_t vert_shader = gfx_shader_load(GL_VERTEX_SHADER, vert_filepath);
  u_int32_t frag_shader = gfx_shader_load(GL_FRAGMENT_SHADER, frag_filepath);
  assert(vert_shader != 0);
  assert(frag_shader != 0);
  // Attach
  GLCall(glAttachShader(id, vert_shader));
  GLCall(glAttachShader(id, frag_shader));
  // Link and validate
  GLCall(glLinkProgram(id));
  GLCall(glValidateProgram(id));
  // Shaders are linked, we can delete the intermidiate
  GLCall(glDeleteShader(vert_shader));
  GLCall(glDeleteShader(frag_shader));

  GLCall(glUseProgram(id));

  GFX_Shader shader = {
      .renderer_id = id,
      .vert_filepath = vert_filepath,
      .frag_filepath = frag_filepath,
  };
  assert(shader.renderer_id != 0);
  return shader;
}

void gfx_shader_destroy(const GFX_Shader *shader) {
  GLCall(glDeleteProgram(shader->renderer_id));
}

void gfx_shader_bind(const GFX_Shader *shader) {
  GLCall(glUseProgram(shader->renderer_id));
}

void gfx_shader_unbind() { GLCall(glUseProgram(0);) }

u_int32_t gfx_shader_uniform_get_location(const GFX_Shader *shader,
                                          const char *name) {
  GLCall(u_int32_t location = glGetUniformLocation(shader->renderer_id, name));
  return location;
}

void gfx_shader_uniform_set_int(const GFX_Shader *shader, const char *name,
                                int v) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniform1i(location, v));
}

void gfx_shader_uniform_set_float(const GFX_Shader *shader, const char *name,
                                  float v) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniform1f(location, v));
}

void gfx_shader_uniform_set_vec2(const GFX_Shader *shader, const char *name,
                                 vec2 v) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniform2fv(location, 1, v));
}

void gfx_shader_uniform_set_vec3(const GFX_Shader *shader, const char *name,
                                 vec3 v) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniform3fv(location, 1, v));
}

void gfx_shader_uniform_set_vec4(const GFX_Shader *shader, const char *name,
                                 vec4 v) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniform4fv(location, 1, v));
}

void gfx_shader_uniform_set_mat4(const GFX_Shader *shader, const char *name,
                                 int transpose, mat4 value) {
  gfx_shader_bind(shader);
  u_int32_t location = gfx_shader_uniform_get_location(shader, name);
  assert(location != -1);
  GLCall(glUniformMatrix4fv(location, 1, transpose, *value));
}
