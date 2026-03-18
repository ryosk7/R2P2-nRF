#ifndef R2P2_NRF52_USB_H
#define R2P2_NRF52_USB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  R2P2_USB_CHANNEL_CONSOLE = 0,
  R2P2_USB_CHANNEL_DATA = 1,
} r2p2_usb_channel_t;

void r2p2_usb_init(void);
void r2p2_usb_task(void);
bool r2p2_usb_connected(void);
bool r2p2_usb_channel_connected(r2p2_usb_channel_t channel);
size_t r2p2_usb_write(r2p2_usb_channel_t channel, const uint8_t *data, size_t length);
size_t r2p2_usb_bytes_available(r2p2_usb_channel_t channel);
int r2p2_usb_read(r2p2_usb_channel_t channel);
void r2p2_usb_disconnect(void);

#endif
