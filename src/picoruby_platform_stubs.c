#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "machine.h"

void * __exidx_start = 0;
void * __exidx_end = 0;

void
FILE_physical_address(void *fp, uint8_t **addr)
{
  (void)fp;
  if (addr != NULL) {
    *addr = NULL;
  }
}

int
FILE_sector_size(void)
{
  return 512;
}

int
clock_gettime(clockid_t clk_id, struct timespec *tp)
{
  (void)clk_id;
  if (tp == NULL) {
    return -1;
  }

  if (Machine_get_hwclock(tp)) {
    return 0;
  }

  tp->tv_sec = (time_t)(Machine_uptime_us() / 1000000u);
  tp->tv_nsec = 0;
  return 0;
}
