#include "hal.h"
#include "r2p2_nrf52_usb.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"

enum {
  HAL_STDIN_BUFFER_SIZE = 128,
};

#ifndef MRBC_TICK_UNIT
#define MRBC_TICK_UNIT 10
#endif

static uint8_t hal_stdin_buffer[HAL_STDIN_BUFFER_SIZE];
static uint16_t hal_stdin_head;
static uint16_t hal_stdin_tail;
static bool hal_tick_timer_started;

APP_TIMER_DEF(hal_tick_timer);

/*
 * The USB pump's owner.
 *
 * app_usbd_event_queue_process() was reached only from paths a Ruby
 * program has to actively take -- console read, console write, sleep.
 * Any code that does not touch stdio (a compute loop, sleep 10) left the
 * 32-entry event queue undrained while the host kept posting to it; MSC
 * alone is enough with a mounted volume. app_usbd discards on overflow
 * without telling anyone, and a lost RX_DONE wedges rx_pending, killing
 * console input until reset.
 *
 * Servicing it from the 10 ms tick gives it one guaranteed owner. This
 * runs in the app_timer handler -- thread level, not the USBD ISR --
 * because APP_USBD_CONFIG_EVENT_QUEUE_ENABLE splits those contexts, so
 * class callbacks still run where they always did. RP2040 solved the
 * same problem with a low-priority pump IRQ plus a 1 ms backstop
 * (picoruby-machine/ports/rp2040/machine.c); the tick is this part's
 * equivalent backstop.
 */
static void hal_tick_timer_handler(void *context) {
  (void)context;
  mrbc_tick();
  r2p2_usb_task();
}

static bool hal_stdin_empty(void) {
  return hal_stdin_head == hal_stdin_tail;
}

static bool hal_stdin_full(void) {
  return (uint16_t)(hal_stdin_head + 1u) % HAL_STDIN_BUFFER_SIZE == hal_stdin_tail;
}

void mrbc_hal_init(void) {
  if (hal_tick_timer_started) {
    return;
  }

  ret_code_t ret = app_timer_create(&hal_tick_timer,
    APP_TIMER_MODE_REPEATED,
    hal_tick_timer_handler);
  if (ret != NRF_SUCCESS && ret != NRF_ERROR_INVALID_STATE) {
    APP_ERROR_CHECK(ret);
  }

  ret = app_timer_start(hal_tick_timer,
    APP_TIMER_TICKS(MRBC_TICK_UNIT),
    NULL);
  if (ret != NRF_SUCCESS && ret != NRF_ERROR_INVALID_STATE) {
    APP_ERROR_CHECK(ret);
  }

  hal_tick_timer_started = true;
}

void mrbc_hal_enable_irq(void) {
  __enable_irq();
}

void mrbc_hal_disable_irq(void) {
  __disable_irq();
}

void mrbc_hal_idle_cpu(void) {
  __WFE();
}

int mrbc_hal_write(int fd, const void *buf, int nbytes) {
  (void)fd;
  const uint8_t *src = (const uint8_t *)buf;
  static const uint8_t crlf[] = "\r\n";
  int start = 0;

  for (int i = 0; i < nbytes; i++) {
    if (src[i] != '\n') {
      continue;
    }

    if (start < i) {
      r2p2_usb_task();
      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE, &src[start], (size_t)(i - start));
    }

    r2p2_usb_task();
    r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE, crlf, sizeof(crlf) - 1);
    start = i + 1;
  }

  if (start < nbytes) {
    r2p2_usb_task();
    r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE, &src[start], (size_t)(nbytes - start));
  }

  return nbytes;
}

int mrbc_hal_flush(int fd) {
  (void)fd;
  return 0;
}

void mrbc_hal_abort(const char *s) {
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

int picorb_hal_getchar(void) {
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
