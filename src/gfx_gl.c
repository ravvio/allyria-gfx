#include "../include/gfx/gfx_gl.h"

void gfx_gl_errors_clear() {
  while (glGetError() != GL_NO_ERROR) {
  }
}

int gfx_gl_errors_check() {
  int ok = 0;
  for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
    fprintf(stderr, "[e] OpenGL Error %d", err);
    ok = -1;
  }
  return ok;
}

int gfx_gl_errors_log(const char *function, const char *file, int line) {
  int ok = 0;
  for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
    fprintf(stderr, "[e] OpenGL Error %x\n| %s\n| at %s:%d\n", err, function,
            file, line);
    ok = -1;
  }
  return ok;
}

u_int32_t gfx_gl_type_get_size(u_int32_t type) {
  switch (type) {
  case GL_FLOAT:
    return sizeof(GLfloat);
  }
  return 0;
}

GLenum gfx_gl_wrap(enum GFX_Wrap wrap) {
  switch (wrap) {
  case GFX_WRAP_REPEAT:
    return GL_REPEAT;
  case GFX_WRAP_MIRROR_REPEAT:
    return GL_MIRRORED_REPEAT;
  case GFX_WRAP_CLAMP_EDGE:
    return GL_CLAMP_TO_EDGE;
  case GFX_WRAP_CLAMP_BORDER:
    return GL_CLAMP_TO_BORDER;
  }
}

GLenum gfx_gl_filter(enum GFX_TextureFilter filter) {
  switch (filter) {
  case GFX_TEXTURE_FILTER_LINEAR:
    return GL_LINEAR;
  case GFX_TEXTURE_FILTER_NEAREST:
    return GL_NEAREST;
  }
}

GLenum gfx_gl_min_filter(enum GFX_TextureFilter filter) {
  switch (filter) {
  case GFX_TEXTURE_FILTER_LINEAR:
    return GL_LINEAR_MIPMAP_LINEAR;
  case GFX_TEXTURE_FILTER_NEAREST:
    return GL_LINEAR_MIPMAP_NEAREST;
  }
}
