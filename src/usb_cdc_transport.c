#include "usb_cdc_transport.h"

#include <string.h>

#include "r2p2_config.h"
#include "r2p2_nrf52_usb.h"

#include "tusb.h"

#if CFG_TUD_CDC != 2
#error CFG_TUD_CDC must be exactly 2
#endif

static const uint8_t usb_cdc_descriptor_template[] = {
  0x08, 0x0B, 0xFF, 0x02, 0x02, 0x02, 0x00, 0x00,
  0x09, 0x04, 0xFF, 0x00, 0x01, 0x02, 0x02, 0x00, 0xFF,
  0x05, 0x24, 0x00, 0x10, 0x01,
  0x05, 0x24, 0x01, 0x01, 0xFF,
  0x04, 0x24, 0x02, 0x02,
  0x05, 0x24, 0x06, 0xFF, 0xFF,
  0x07, 0x05, 0xFF, 0x03, 0x40, 0x00, 0x10,
  0x09, 0x04, 0xFF, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x05,
  0x07, 0x05, 0xFF, 0x02,
#if R2P2_USB_DEVICE_HIGH_SPEED == 1
  0x00, 0x02,
#else
  0x40, 0x00,
#endif
  0x00,
  0x07, 0x05, 0xFF, 0x02,
#if R2P2_USB_DEVICE_HIGH_SPEED == 1
  0x00, 0x02,
#else
  0x40, 0x00,
#endif
  0x00,
};

enum {
  CDC_FIRST_INTERFACE_INDEX = 2,
  CDC_COMM_INTERFACE_INDEX = 10,
  CDC_COMM_INTERFACE_STRING_INDEX = 16,
  CDC_CALL_MANAGEMENT_DATA_INTERFACE_INDEX = 26,
  CDC_UNION_MASTER_INTERFACE_INDEX = 34,
  CDC_UNION_SLAVE_INTERFACE_INDEX = 35,
  CDC_CONTROL_IN_ENDPOINT_INDEX = 38,
  CDC_DATA_INTERFACE_INDEX = 45,
  CDC_DATA_INTERFACE_STRING_INDEX = 51,
  CDC_DATA_OUT_ENDPOINT_INDEX = 54,
  CDC_DATA_IN_ENDPOINT_INDEX = 61,
};

static const char console_cdc_comm_interface_name[] = R2P2_USB_INTERFACE_NAME " CDC control";
static const char data_cdc_comm_interface_name[] = R2P2_USB_INTERFACE_NAME " CDC2 control";
static const char console_cdc_data_interface_name[] = R2P2_USB_INTERFACE_NAME " CDC data";
static const char data_cdc_data_interface_name[] = R2P2_USB_INTERFACE_NAME " CDC2 data";

static bool usb_cdc_console_is_enabled;
static bool usb_cdc_data_is_enabled;
static uint8_t usb_cdc_console_channel;
static uint8_t usb_cdc_data_channel;

void usb_cdc_set_defaults(void) {
  usb_cdc_enable(R2P2_USB_CDC_CONSOLE_ENABLED_DEFAULT, R2P2_USB_CDC_DATA_ENABLED_DEFAULT);
}

bool usb_cdc_console_enabled(void) { return usb_cdc_console_is_enabled; }
bool usb_cdc_data_enabled(void) { return usb_cdc_data_is_enabled; }
uint8_t usb_cdc_console_index(void) { return usb_cdc_console_channel; }
uint8_t usb_cdc_data_index(void) { return usb_cdc_data_channel; }
size_t usb_cdc_descriptor_length(void) { return sizeof(usb_cdc_descriptor_template); }

size_t usb_cdc_add_descriptor(uint8_t *descriptor_buf, descriptor_counts_t *descriptor_counts,
  uint8_t *current_interface_string, bool console) {
  memcpy(descriptor_buf, usb_cdc_descriptor_template, sizeof(usb_cdc_descriptor_template));

  descriptor_buf[CDC_FIRST_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_buf[CDC_COMM_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_buf[CDC_UNION_MASTER_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_counts->current_interface++;

  descriptor_buf[CDC_CALL_MANAGEMENT_DATA_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_buf[CDC_UNION_SLAVE_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_buf[CDC_DATA_INTERFACE_INDEX] = descriptor_counts->current_interface;
  descriptor_counts->current_interface++;

  descriptor_buf[CDC_CONTROL_IN_ENDPOINT_INDEX] = console ? R2P2_USB_CDC0_EP_NOTIFICATION : R2P2_USB_CDC1_EP_NOTIFICATION;
  descriptor_buf[CDC_DATA_OUT_ENDPOINT_INDEX] = console ? R2P2_USB_CDC0_EP_OUT : R2P2_USB_CDC1_EP_OUT;
  descriptor_buf[CDC_DATA_IN_ENDPOINT_INDEX] = console ? R2P2_USB_CDC0_EP_IN : R2P2_USB_CDC1_EP_IN;
  descriptor_counts->num_in_endpoints += 2;
  descriptor_counts->num_out_endpoints += 1;

  r2p2_usb_add_interface_string(*current_interface_string,
    console ? console_cdc_comm_interface_name : data_cdc_comm_interface_name);
  descriptor_buf[CDC_COMM_INTERFACE_STRING_INDEX] = *current_interface_string;
  (*current_interface_string)++;

  r2p2_usb_add_interface_string(*current_interface_string,
    console ? console_cdc_data_interface_name : data_cdc_data_interface_name);
  descriptor_buf[CDC_DATA_INTERFACE_STRING_INDEX] = *current_interface_string;
  (*current_interface_string)++;

  return sizeof(usb_cdc_descriptor_template);
}

bool usb_cdc_disable(void) {
  return usb_cdc_enable(false, false);
}

bool usb_cdc_enable(bool console, bool data) {
  if (tud_connected()) {
    return false;
  }

  uint8_t idx = 0;
  usb_cdc_console_is_enabled = console;
  if (console) {
    usb_cdc_console_channel = idx++;
  }

  usb_cdc_data_is_enabled = data;
  if (data) {
    usb_cdc_data_channel = idx;
  }

  return true;
}
