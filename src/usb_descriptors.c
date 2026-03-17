#include "r2p2_nrf52_usb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "r2p2_config.h"
#include "usb_cdc_transport.h"

#include "tusb.h"

static const char nibble_to_hex_upper[] = "0123456789ABCDEF";

#define MAX_INTERFACE_STRINGS 16
#define R2P2_UID_LENGTH 8

typedef union {
  const char *char_str;
  const uint16_t *descriptor;
} interface_string_t;

static interface_string_t collected_interface_strings[MAX_INTERFACE_STRINGS];
static size_t collected_interface_strings_length;
static uint8_t current_interface_string;

static uint8_t *device_descriptor;
static uint8_t *configuration_descriptor;
static uint16_t *string_descriptors;
static char serial_number_hex_string[R2P2_UID_LENGTH * 2 + 1];

enum {
  DEVICE_VID_LO_INDEX = 8,
  DEVICE_VID_HI_INDEX = 9,
  DEVICE_PID_LO_INDEX = 10,
  DEVICE_PID_HI_INDEX = 11,
  DEVICE_MANUFACTURER_STRING_INDEX = 14,
  DEVICE_PRODUCT_STRING_INDEX = 15,
  DEVICE_SERIAL_NUMBER_STRING_INDEX = 16,
  CONFIG_TOTAL_LENGTH_LO_INDEX = 2,
  CONFIG_TOTAL_LENGTH_HI_INDEX = 3,
  CONFIG_NUM_INTERFACES_INDEX = 4,
};

static const uint8_t device_descriptor_template[] = {
  0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x01,
};

static const uint8_t configuration_descriptor_template[] = {
  0x09, 0x02, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x80 | TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 0x32,
};

static const uint16_t language_id[] = {0x0304, 0x0409};

static void r2p2_get_uid(uint8_t *raw_id) {
  for (size_t i = 0; i < R2P2_UID_LENGTH; i++) {
    raw_id[i] = (uint8_t)(0xA0 + i);
  }
}

void r2p2_usb_add_interface_string(uint8_t interface_string_index, const char str[]) {
  if (interface_string_index > MAX_INTERFACE_STRINGS) {
    return;
  }
  collected_interface_strings[interface_string_index].char_str = str;
  collected_interface_strings_length += strlen(str);
}

static bool build_device_descriptor(const usb_identification_t *identification) {
  device_descriptor = malloc(sizeof(device_descriptor_template));
  if (device_descriptor == NULL) {
    return false;
  }
  memcpy(device_descriptor, device_descriptor_template, sizeof(device_descriptor_template));

  device_descriptor[DEVICE_VID_LO_INDEX] = identification->vid & 0xFF;
  device_descriptor[DEVICE_VID_HI_INDEX] = identification->vid >> 8;
  device_descriptor[DEVICE_PID_LO_INDEX] = identification->pid & 0xFF;
  device_descriptor[DEVICE_PID_HI_INDEX] = identification->pid >> 8;

  r2p2_usb_add_interface_string(current_interface_string, identification->manufacturer_name);
  device_descriptor[DEVICE_MANUFACTURER_STRING_INDEX] = current_interface_string++;
  r2p2_usb_add_interface_string(current_interface_string, identification->product_name);
  device_descriptor[DEVICE_PRODUCT_STRING_INDEX] = current_interface_string++;
  r2p2_usb_add_interface_string(current_interface_string, serial_number_hex_string);
  device_descriptor[DEVICE_SERIAL_NUMBER_STRING_INDEX] = current_interface_string++;
  return true;
}

static bool build_configuration_descriptor(void) {
  size_t total_descriptor_length = sizeof(configuration_descriptor_template);
  if (usb_cdc_console_enabled()) {
    total_descriptor_length += usb_cdc_descriptor_length();
  }
  if (usb_cdc_data_enabled()) {
    total_descriptor_length += usb_cdc_descriptor_length();
  }

  configuration_descriptor = malloc(total_descriptor_length);
  if (configuration_descriptor == NULL) {
    return false;
  }

  memcpy(configuration_descriptor, configuration_descriptor_template, sizeof(configuration_descriptor_template));
  configuration_descriptor[CONFIG_TOTAL_LENGTH_LO_INDEX] = total_descriptor_length & 0xFF;
  configuration_descriptor[CONFIG_TOTAL_LENGTH_HI_INDEX] = (total_descriptor_length >> 8) & 0xFF;

  descriptor_counts_t descriptor_counts = {
    .current_interface = 0,
    .current_endpoint = 1,
    .num_in_endpoints = 1,
    .num_out_endpoints = 1,
  };

  uint8_t *descriptor_buf_remaining = configuration_descriptor + sizeof(configuration_descriptor_template);
  if (usb_cdc_console_enabled()) {
    descriptor_buf_remaining += usb_cdc_add_descriptor(descriptor_buf_remaining, &descriptor_counts,
      &current_interface_string, true);
  }
  if (usb_cdc_data_enabled()) {
    descriptor_buf_remaining += usb_cdc_add_descriptor(descriptor_buf_remaining, &descriptor_counts,
      &current_interface_string, false);
  }

  configuration_descriptor[CONFIG_NUM_INTERFACES_INDEX] = descriptor_counts.current_interface;
  if (descriptor_counts.current_endpoint > R2P2_USB_NUM_ENDPOINT_PAIRS ||
      descriptor_counts.num_in_endpoints > R2P2_USB_NUM_IN_ENDPOINTS ||
      descriptor_counts.num_out_endpoints > R2P2_USB_NUM_OUT_ENDPOINTS) {
    return false;
  }
  return true;
}

static bool build_interface_string_table(void) {
  string_descriptors = malloc(current_interface_string * 2 + collected_interface_strings_length * 2);
  if (string_descriptors == NULL) {
    return false;
  }

  uint16_t *string_descriptor = string_descriptors;
  collected_interface_strings[0].descriptor = language_id;

  for (uint8_t string_index = 1; string_index < current_interface_string; string_index++) {
    const char *str = collected_interface_strings[string_index].char_str;
    size_t str_len = strlen(str);
    uint8_t descriptor_size_words = 1 + str_len;
    uint8_t descriptor_size_bytes = descriptor_size_words * 2;
    string_descriptor[0] = 0x0300 | descriptor_size_bytes;
    for (size_t i = 0; i < str_len; i++) {
      string_descriptor[i + 1] = str[i];
    }
    collected_interface_strings[string_index].descriptor = string_descriptor;
    string_descriptor += descriptor_size_words;
  }
  return true;
}

bool r2p2_usb_build_descriptors(const usb_identification_t *identification) {
  uint8_t raw_id[R2P2_UID_LENGTH];

  r2p2_get_uid(raw_id);
  for (size_t i = 0; i < R2P2_UID_LENGTH; i++) {
    for (int j = 0; j < 2; j++) {
      uint8_t nibble = (raw_id[i] >> (j * 4)) & 0x0f;
      serial_number_hex_string[i * 2 + (1 - j)] = nibble_to_hex_upper[nibble];
    }
  }
  serial_number_hex_string[sizeof(serial_number_hex_string) - 1] = '\0';

  current_interface_string = 1;
  collected_interface_strings_length = 0;
  return build_device_descriptor(identification) &&
    build_configuration_descriptor() &&
    build_interface_string_table();
}

uint8_t const *tud_descriptor_device_cb(void) {
  return device_descriptor;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return configuration_descriptor;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  if (index > MAX_INTERFACE_STRINGS) {
    return NULL;
  }
  return collected_interface_strings[index].descriptor;
}
