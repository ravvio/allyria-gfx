// Checks the CPU mipmap generation of src/graphics/gfx_texture.c, no GPU needed

#undef NDEBUG
#include "../include/gfx/graphics/gfx_wgpu.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  // Mip chain goes down to 1x1, 1 + floor(log2(max(w, h)))
  assert(gfx_texture_mip_count(1, 1) == 1);
  assert(gfx_texture_mip_count(2, 1) == 2);
  assert(gfx_texture_mip_count(256, 256) == 9);
  assert(gfx_texture_mip_count(123, 456) == 9);

  uint8_t dst[4];

  // 2x2 -> 1x1 averages the 4 pixels, rounded
  uint8_t square[2 * 2 * 4] = {
      0, 0, 0, 0, 255, 255, 255, 255, //
      0, 0, 0, 0, 255, 255, 255, 255, //
  };
  gfx_texture_downsample(square, 2, 2, dst, 1, 1);
  assert(dst[0] == 128 && dst[3] == 128);

  // Odd width drops the last column: 3x2 -> 1x1 uses columns 0 and 1
  uint8_t odd[3 * 2 * 4] = {
      10, 0, 0, 0, 30, 0, 0, 0, 200, 0, 0, 0, //
      10, 0, 0, 0, 30, 0, 0, 0, 200, 0, 0, 0, //
  };
  gfx_texture_downsample(odd, 3, 2, dst, 1, 1);
  assert(dst[0] == 20);

  // 1 pixel tall sources clamp the second row: 2x1 -> 1x1
  uint8_t thin[2 * 1 * 4] = {100, 0, 0, 0, 200, 0, 0, 0};
  gfx_texture_downsample(thin, 2, 1, dst, 1, 1);
  assert(dst[0] == 150);

  printf("texture mips ok\n");
  return 0;
}
