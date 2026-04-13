#include "../include/gfx/gfx_gl.h"
#include "../include/stb_image.h"

// Load a texture
GFX_Texture2D gfx_texture2d_load(const char *filename,
                                 enum GFX_TextureType type,
                                 enum GFX_TextureFilter filter,
                                 enum GFX_Wrap wrap) {
  // Get image data
  int width, height, channels;
  u_int8_t *data = stbi_load(filename, &width, &height, &channels, 3);

  if (!data) {
    fprintf(stderr, "[e] failed to load texture image: %s\n| %s", filename,
            stbi_failure_reason());
    GFX_Texture2D tex = {
        .width = 0,
        .height = 0,
        .renderer_id = -1,
        .type = 0,
    };
  }

  // Generate texture
  u_int32_t renderer_id;
  glGenTextures(1, &renderer_id);
  glBindTexture(GL_TEXTURE_2D, renderer_id);

  // Texture wrapping and filtering options
  GLenum gl_wrap = gfx_gl_wrap(wrap);
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                         gfx_gl_min_filter(filter)));
  GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                         gfx_gl_filter(filter)));

  // Add image data
  GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, data));
  GLCall(glGenerateMipmap(GL_TEXTURE_2D));

  // Free image data
  stbi_image_free(data);
  // Unbind texture
  gfx_texture2d_unbind();

  GFX_Texture2D tex = {
      .width = width,
      .height = height,
      .renderer_id = renderer_id,
      .type = type,
  };
  fprintf(stderr, "[i] loaded texture %s\n", filename);
  return tex;
}

void gfx_texture2d_destroy(const GFX_Texture2D *texture) {
  GLCall(glDeleteTextures(1, &texture->renderer_id));
}

void gfx_texture2d_bind(const GFX_Texture2D *texture) {
  int unit = GL_TEXTURE0;
  switch (texture->type) {
  case GFX_TEXTURE_TYPE_DIFFUSE:
    break;
  case GFX_TEXTURE_TYPE_SPECULAR:
    unit = GL_TEXTURE1;
    break;
  }
  GLCall(glActiveTexture(unit));
  GLCall(glBindTexture(GL_TEXTURE_2D, texture->renderer_id));
}

void gfx_texture2d_unbind() { GLCall(glBindTexture(GL_TEXTURE_2D, 0)); }
