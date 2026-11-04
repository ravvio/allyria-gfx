// Checks the 2D workgroup split, no GPU needed

#undef NDEBUG
#include "gfx_internal.h"
#include <assert.h>
#include <stdio.h>

static void check(uint64_t groups, uint32_t limit, uint32_t x, uint32_t y) {
  uint32_t gx, gy;
  gfx_compute_groups_split(groups, limit, &gx, &gy);
  assert(gx == x && gy == y);
  // Enough workgroups, less than one row too many
  assert((uint64_t)gx * gy >= groups && (uint64_t)gx * gy < groups + gx);
}

int main(void) {
  // Fits in one dimension
  check(1, 65535, 1, 1);
  check(65535, 65535, 65535, 1);

  // One past the limit takes a second row
  check(65536, 65535, 65535, 2);

  // 2^24 groups (2^32 invocations of 256)
  check(1u << 24, 65535, 65535, 257);

  // Nothing to do
  uint32_t gx, gy;
  gfx_compute_groups_split(0, 65535, &gx, &gy);
  assert(gx == 0 && gy == 1);

  printf("compute groups ok\n");
  return 0;
}
