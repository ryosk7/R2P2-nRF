#include "serial_transport.h"

#include <stdarg.h>
#include <string.h>

#include "r2p2_nrf52_usb.h"
#include "usb_cdc_transport.h"

static bool serial_console_write_disabled;

void serial_early_init(void) {
}

void serial_init(void) {
}

bool serial_connected(void) {
  return usb_cdc_transport_channel_connected(R2P2_USB_CHANNEL_CONSOLE);
}

char serial_read(void) {
  int value = usb_cdc_transport_read(R2P2_USB_CHANNEL_CONSOLE);
  if (value >= 0) {
    return (char)value;
  }
  return -1;
}

uint32_t serial_bytes_available(void) {
  return (uint32_t)usb_cdc_transport_bytes_available(R2P2_USB_CHANNEL_CONSOLE);
}

uint32_t serial_write_substring(const char *text, uint32_t length) {
  if (length == 0 || serial_console_write_disabled) {
    return length;
  }

  return (uint32_t)usb_cdc_transport_write(R2P2_USB_CHANNEL_CONSOLE, (const uint8_t *)text, length);
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
