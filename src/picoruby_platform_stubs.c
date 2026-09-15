#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

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

/*
 * The wall clock lives in picoruby-machine's nRF52 port, on RTC0 with an
 * epoch offset. Until something sets it -- NTP, a shell command, a value
 * from flash -- Machine_get_hwclock reports failure, and uptime is the
 * only honest answer left. Callers that care can tell the two apart by
 * comparing against a plausible epoch; callers that only measure
 * intervals do not need to.
 */
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

  uint64_t us = Machine_uptime_us();
  tp->tv_sec = (time_t)(us / 1000000u);
  tp->tv_nsec = (long)((us % 1000000u) * 1000u);
  return 0;
}

/*
 * newlib routes gettimeofday() here. picoruby-littlefs stamps file mtimes
 * through it and picoruby-mbedtls times its DTLS retransmissions with it,
 * so without this both silently get whatever newlib's failing stub leaves
 * behind.
 *
 * Only the reading half is provided. Setting the clock goes through
 * Machine.set_hwclock, which reaches Machine_set_hwclock directly -- there
 * is no caller anywhere for clock_settime or settimeofday, and adding them
 * would be surface nothing can reach.
 */
int
_gettimeofday(struct timeval *tv, void *tzvp)
{
  struct timespec ts;

  (void)tzvp;   /* no timezone database on this platform */
  if (tv == NULL) {
    return -1;
  }
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
    return -1;
  }
  tv->tv_sec = ts.tv_sec;
  tv->tv_usec = (suseconds_t)(ts.tv_nsec / 1000);
  return 0;
}
