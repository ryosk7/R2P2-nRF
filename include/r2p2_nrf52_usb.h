#ifndef R2P2_NRF52_USB_H
#define R2P2_NRF52_USB_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t vid;
  uint16_t pid;
  char manufacturer_name[64];
  char product_name[64];
} usb_identification_t;

void r2p2_usb_init(void);
void r2p2_usb_task(void);
bool r2p2_usb_connected(void);
bool r2p2_usb_build_descriptors(const usb_identification_t *identification);
void r2p2_usb_add_interface_string(uint8_t interface_string_index, const char str[]);
void r2p2_usb_disconnect(void);

#endif
