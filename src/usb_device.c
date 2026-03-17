#include "r2p2_nrf52_usb.h"

#include "usb_cdc_transport.h"

#include "tusb.h"

__attribute__((weak)) void r2p2_reset_to_bootloader(void) {
}

__attribute__((weak)) void r2p2_wake_main_task(void) {
}

void r2p2_usb_disconnect(void) {
  tud_disconnect();
}

void tud_mount_cb(void) {
}

void tud_umount_cb(void) {
}

void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
}

void tud_resume_cb(void) {
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;

  if (!dtr) {
    cdc_line_coding_t coding;
    tud_cdc_get_line_coding(&coding);
    if (coding.bit_rate == 1200) {
      r2p2_reset_to_bootloader();
    }
  }
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms) {
  (void)duration_ms;
  if (usb_cdc_console_enabled() && itf == 0) {
    r2p2_wake_main_task();
  }
}

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;
  r2p2_wake_main_task();
}
