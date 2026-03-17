#include "serial_transport.h"

#include <stdarg.h>
#include <string.h>

#include "r2p2_config.h"
#include "usb_cdc_transport.h"

#include "tusb.h"

static bool serial_console_write_disabled;

void serial_early_init(void) {
}

void serial_init(void) {
}

bool serial_connected(void) {
#if R2P2_TINYUSB && R2P2_USB_DEVICE && R2P2_USB_CDC
  return usb_cdc_console_enabled() && tud_cdc_connected();
#else
  return false;
#endif
}

char serial_read(void) {
#if R2P2_TINYUSB && R2P2_USB_DEVICE && R2P2_USB_CDC
  if (usb_cdc_console_enabled() && tud_cdc_connected() && tud_cdc_available() > 0) {
    return (char)tud_cdc_read_char();
  }
#endif
  return -1;
}

uint32_t serial_bytes_available(void) {
#if R2P2_TINYUSB && R2P2_USB_DEVICE && R2P2_USB_CDC
  if (usb_cdc_console_enabled() && tud_cdc_connected()) {
    return tud_cdc_available();
  }
#endif
  return 0;
}

uint32_t serial_write_substring(const char *text, uint32_t length) {
  if (length == 0 || serial_console_write_disabled) {
    return length;
  }

#if R2P2_TINYUSB && R2P2_USB_DEVICE && R2P2_USB_CDC
  if (usb_cdc_console_enabled() && tud_cdc_connected()) {
    uint32_t written = 0;
    while (written < length) {
      written += tud_cdc_write(text + written, length - written);
      tud_task();
      tud_cdc_write_flush();
    }
  }
#endif
  return length;
}

void serial_write(const char *text) {
  serial_write_substring(text, strlen(text));
}

bool serial_console_write_disable(bool disabled) {
  bool previous = serial_console_write_disabled;
  serial_console_write_disabled = disabled;
  return previous;
}

int console_uart_printf(const char *fmt, ...) {
  (void)fmt;
  return 0;
}
