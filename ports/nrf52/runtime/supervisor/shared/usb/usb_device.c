// This file is part of the R2P2-nRF52 runtime.
//
// SPDX-FileCopyrightText: Copyright (c) 2018 hathach for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/port.h"
#include "supervisor/shared/r2p2_runtime_config.h"
#include "supervisor/usb.h"

#if R2P2_USB_CDC
#include "shared-module/usb_cdc/__init__.h"
#endif

#include "tusb.h"

void usb_disconnect(void) {
    tud_disconnect();
}

// Invoked when device is mounted
void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allows us to perform remote wakeup
// USB Specs: Within 7ms, device must draw an average current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

// Invoked when cdc when line state changed e.g connected/disconnected
// Use to reset to DFU when disconnect with 1200 bps
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;  // interface ID, not used

    // DTR = false is counted as disconnected
    if (!dtr) {
        cdc_line_coding_t coding;
        // Use whichever CDC is itf 0.
        tud_cdc_get_line_coding(&coding);

        if (coding.bit_rate == 1200) {
            reset_to_bootloader();
        }
    }
}


void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms) {
    (void)duration_ms;
    if (usb_cdc_console_enabled() && itf == 0) {
        port_wake_main_task();
    }
}

void tud_cdc_rx_cb(uint8_t itf) {
    (void)itf;
    port_wake_main_task();
}
