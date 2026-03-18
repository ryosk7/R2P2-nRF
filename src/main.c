#include "r2p2_nrf52_usb.h"

int main(void) {
  r2p2_usb_init();
  while (1) {
    r2p2_usb_task();
  }
}
