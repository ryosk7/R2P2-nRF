#include "r2p2_nrf52_usb.h"

#include <string.h>

#include "r2p2_config.h"
#include "serial_transport.h"
#include "usb_cdc_transport.h"

#include "tusb.h"

__attribute__((weak)) void init_usb_hardware(void) {
}

__attribute__((weak)) void post_usb_init(void) {
}

bool r2p2_usb_connected(void) {
  return tud_ready();
}

void r2p2_usb_init(void) {
  usb_identification_t defaults;

  defaults.vid = R2P2_USB_VENDOR_ID;
  defaults.pid = R2P2_USB_PRODUCT_ID;
  strcpy(defaults.manufacturer_name, R2P2_USB_MANUFACTURER);
  strcpy(defaults.product_name, R2P2_USB_PRODUCT);

  usb_cdc_set_defaults();
  if (!r2p2_usb_build_descriptors(&defaults)) {
    return;
  }

  init_usb_hardware();
  tud_init(TUD_OPT_RHPORT);
  post_usb_init();
}

void r2p2_usb_task(void) {
  if (tusb_inited()) {
    tud_task();
    if (usb_cdc_console_enabled()) {
      tud_cdc_write_flush();
    }
  }
}
