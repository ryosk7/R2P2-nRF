#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "app_timer.h"
#include "hal.h"
#include "machine.h"
#include "mrubyc.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "platform.h"
#include "picoruby_runtime.h"
#include "r2p2_nrf52_usb.h"
#if defined(NRF52_THREAD_ENABLED)
#include "openthread_glue.h"
#endif

void picoruby_init_require(mrbc_vm *vm);

volatile int sigint_status = MACHINE_SIG_NONE;

static void runtime_debug(const char *text) {
  r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE, (const uint8_t *)text, strlen(text));
}

void r2p2_reset_to_bootloader(void);

/*
 * Machine.reset_to_bootloader
 *
 * Named for what it does rather than "dfu": picoruby already has a DFU
 * gem, and that one swaps application slots in the filesystem -- it does
 * not reflash the MCU. Conflating the two names would be the confusing
 * choice.
 *
 * Defined from the product rather than from picoruby-machine because
 * entering the bootloader is a board fact, not a chip fact: it depends
 * on which bootloader is installed and which magic it honours. Reopening
 * Machine here keeps picoruby untouched.
 */
static void c_machine_reset_to_bootloader(mrbc_vm *vm, mrbc_value *v, int argc) {
  (void)v;
  (void)argc;
  r2p2_reset_to_bootloader();   /* does not return */
  (void)vm;
}

bool r2p2_picoruby_init_runtime(mrbc_vm *vm) {
  runtime_debug("[r2p2] picoruby_init_require begin\r\n");
  picoruby_init_require(vm);
  runtime_debug("[r2p2] picoruby_init_require done\r\n");

  mrbc_class *machine = mrbc_get_class_by_name("Machine");
  if (machine != NULL) {
    mrbc_define_method(vm, machine, "reset_to_bootloader",
                       c_machine_reset_to_bootloader);
  }
  return true;
}

/* Platform_name comes from picoruby's
 * picoruby-require/ports/nrf52/platform.c, which reports "NRF52" -- the
 * spelling Machine.posix? checks for. That definition is weak, so this
 * one would have kept winning and left RUBY_PLATFORM lowercase, which
 * sends bareword gets/getc down the POSIX path and into IO#fileno. */

/*
 * What a busy wait keeps alive. These loops never return to mrbc_run, so
 * the scheduler hook that normally pumps OpenThread does not run for
 * their whole length -- seconds, for Machine_sleep -- and the radio would
 * go unserviced. USB is pumped here for the same reason.
 */
static void
busy_wait_service(void)
{
  r2p2_usb_task();
#if defined(NRF52_THREAD_ENABLED)
  OT_process();
#endif
}

void Machine_sleep(uint32_t seconds) {
  for (uint32_t i = 0; i < seconds; i++) {
    for (uint32_t j = 0; j < 100; j++) {
      busy_wait_service();
      nrf_delay_ms(10);
    }
  }
}

void Machine_deep_sleep(uint8_t gpio_pin, bool edge, bool high) {
  (void)gpio_pin;
  (void)edge;
  (void)high;
}

void Machine_delay_ms(uint32_t ms) {
  while (ms > 0) {
    uint32_t step = ms > 10 ? 10 : ms;
    busy_wait_service();
    nrf_delay_ms(step);
    ms -= step;
  }
}

void Machine_busy_wait_ms(uint32_t ms) {
  Machine_delay_ms(ms);
}

void Machine_busy_wait_us(uint32_t us) {
  busy_wait_service();
  nrf_delay_us(us);
}

void Machine_tud_task(void) {
  r2p2_usb_task();
}

bool Machine_tud_mounted_q(void) {
  return r2p2_usb_channel_connected(R2P2_USB_CHANNEL_CONSOLE);
}

void Machine_exit(int status) {
  (void)status;
  while (1) {
    r2p2_usb_task();
    __WFE();
  }
}

void Machine_reboot(void) {
  NVIC_SystemReset();
}

uint64_t Machine_uptime_us(void) {
  uint32_t ticks = app_timer_cnt_get();
  return ((uint64_t)ticks * 1000000u * (APP_TIMER_CONFIG_RTC_FREQUENCY + 1u)) / APP_TIMER_CLOCK_FREQ;
}

/* Machine_uptime_formatted comes from picoruby's
 * picoruby-machine/ports/common/machine.c, which every other platform
 * already shares. Defining it here shadowed that with an nRF-only
 * "HH:MM:SS" and dropped the day and centisecond fields. */
