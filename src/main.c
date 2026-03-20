#include "r2p2_nrf52_usb.h"
#include "mrubyc.h"
#include "main_task.c"
#include "hal.h"

#ifndef HEAP_SIZE
#define HEAP_SIZE (1024 * 64)
#endif

static uint8_t heap_pool[HEAP_SIZE];

int main(void) {
  // Initialize Hardware Abstraction Layer
  hal_init();

  // Initialize USB-CDC
  r2p2_usb_init();

  // Initialize mruby/c VM
  mrbc_init(heap_pool, HEAP_SIZE);
  mrbc_create_task(main_task, 0);

  while (1) {
    // Execute USB task
    r2p2_usb_task();

    // Execute mruby/c VM task
    mrbc_tick();
  }
}
