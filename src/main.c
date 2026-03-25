#include "r2p2_nrf52_usb.h"

int main(void) {
  static const char usb_boot_banner[] = "[r2p2] usb console open\r\n";
  bool console_banner_sent = false;

  r2p2_usb_init();
  while (1) {
    r2p2_usb_task();

    if (!console_banner_sent &&
        r2p2_usb_channel_connected(R2P2_USB_CHANNEL_CONSOLE)) {
      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)usb_boot_banner,
        sizeof(usb_boot_banner) - 1);
      console_banner_sent = true;
    }
  }
}
