#include "hal.h"
#include "r2p2_nrf52_usb.h"
#include "nrf.h"

enum {
  HAL_STDIN_BUFFER_SIZE = 128,
};

static uint8_t hal_stdin_buffer[HAL_STDIN_BUFFER_SIZE];
static uint16_t hal_stdin_head;
static uint16_t hal_stdin_tail;

static bool hal_stdin_empty(void) {
  return hal_stdin_head == hal_stdin_tail;
}

static bool hal_stdin_full(void) {
  return (uint16_t)(hal_stdin_head + 1u) % HAL_STDIN_BUFFER_SIZE == hal_stdin_tail;
}

void hal_init(void) {
  /* USB is initialized in main before hal_init. Nothing to do here. */
}

void hal_enable_irq(void) {
  __enable_irq();
}

void hal_disable_irq(void) {
  __disable_irq();
}

void hal_idle_cpu(void) {
  __WFE();
}

int hal_write(int fd, const void *buf, int nbytes) {
  (void)fd;
  r2p2_usb_task();
  return (int)r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE, (const uint8_t *)buf, (size_t)nbytes);
}

int hal_flush(int fd) {
  (void)fd;
  return 0;
}

void hal_abort(const char *s) {
  (void)s;
  while (1) {
    r2p2_usb_task();
    __WFE();
  }
}

int hal_read_available(void) {
  if (!hal_stdin_empty()) {
    return 1;
  }

  r2p2_usb_task();
  return r2p2_usb_bytes_available(R2P2_USB_CHANNEL_CONSOLE) > 0 ? 1 : 0;
}

int hal_getchar(void) {
  if (!hal_stdin_empty()) {
    uint8_t ch = hal_stdin_buffer[hal_stdin_tail];
    hal_stdin_tail = (uint16_t)(hal_stdin_tail + 1u) % HAL_STDIN_BUFFER_SIZE;
    return ch;
  }

  r2p2_usb_task();
  if (r2p2_usb_bytes_available(R2P2_USB_CHANNEL_CONSOLE) == 0) {
    return HAL_GETCHAR_NODATA;
  }
  return r2p2_usb_read(R2P2_USB_CHANNEL_CONSOLE);
}

bool hal_stdin_push(uint8_t ch) {
  if (hal_stdin_full()) {
    return false;
  }

  hal_stdin_buffer[hal_stdin_head] = ch;
  hal_stdin_head = (uint16_t)(hal_stdin_head + 1u) % HAL_STDIN_BUFFER_SIZE;
  return true;
}
