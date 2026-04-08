#include <stdio.h>
#include <sys/types.h>

#include "../include/gfx/gfx.h"

void gfx_errors_clear() {
  while (glGetError() != GL_NO_ERROR) {
  }
}

int gfx_errors_check() {
  int ok = 0;
  for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
    fprintf(stderr, "[e] OpenGL Error %d", err);
    ok = -1;
  }
  return ok;
}

int gfx_errors_log(const char *function, const char *file, int line) {
  int ok = 0;
  for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
    fprintf(stderr, "[e] OpenGL Error x%x\n| %s\n| at %s:%d\n", err, function,
            file, line);
    ok = -1;
  }
  return ok;
}

u_int32_t gfx_gltype_get_size(u_int32_t type) {
  switch (type) {
  case GL_FLOAT:
    return sizeof(GLfloat);
  }
  return 0;
}
