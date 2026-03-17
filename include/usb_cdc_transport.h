#ifndef USB_CDC_TRANSPORT_H
#define USB_CDC_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t current_interface;
  uint8_t current_endpoint;
  uint8_t num_in_endpoints;
  uint8_t num_out_endpoints;
} descriptor_counts_t;

void usb_cdc_set_defaults(void);
bool usb_cdc_enable(bool console, bool data);
bool usb_cdc_disable(void);

bool usb_cdc_console_enabled(void);
bool usb_cdc_data_enabled(void);
uint8_t usb_cdc_console_index(void);
uint8_t usb_cdc_data_index(void);

size_t usb_cdc_descriptor_length(void);
size_t usb_cdc_add_descriptor(uint8_t *descriptor_buf,
  descriptor_counts_t *descriptor_counts,
  uint8_t *current_interface_string,
  bool console);

#endif
