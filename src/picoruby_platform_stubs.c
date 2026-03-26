#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "machine.h"

void * __exidx_start = 0;
void * __exidx_end = 0;

void
ENV_get_key_value(char **key, size_t *key_len, char **value, size_t *value_len)
{
  *key = NULL;
  *key_len = 0;
  *value = NULL;
  *value_len = 0;
}

int
ENV_unsetenv(const char *name)
{
  (void)name;
  return 0;
}

int
ENV_setenv(const char *name, const char *value, int override)
{
  (void)name;
  (void)value;
  (void)override;
  return 0;
}

void
Watchdog_enable(uint32_t delay_ms, bool pause_on_debug)
{
  (void)delay_ms;
  (void)pause_on_debug;
}

void
Watchdog_disable(void)
{
}

void
Watchdog_reboot(uint32_t delay_ms)
{
  Machine_delay_ms(delay_ms);
  Machine_reboot();
}

void
Watchdog_start_tick(uint32_t cycles)
{
  (void)cycles;
}

void
Watchdog_update(void)
{
}

bool
Watchdog_caused_reboot(void)
{
  return false;
}

bool
Watchdog_enable_caused_reboot(void)
{
  return false;
}

uint32_t
Watchdog_get_count(void)
{
  return 0;
}

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
