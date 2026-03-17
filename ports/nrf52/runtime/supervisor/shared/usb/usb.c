// This file is part of the R2P2-nRF52 runtime.
//
// SPDX-FileCopyrightText: Copyright (c) 2018 hathach for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/background_callback.h"
#include "supervisor/linker.h"
#include "supervisor/port.h"
#include "supervisor/shared/r2p2_runtime_config.h"
#include "supervisor/shared/tick.h"
#include "supervisor/usb.h"
#include "shared/readline/readline.h"

#if R2P2_USB_DEVICE
#include "shared-bindings/supervisor/__init__.h"

#if R2P2_USB_CDC
#include "shared-module/usb_cdc/__init__.h"
#endif

#endif

#include "tusb.h"

#if CFG_TUSB_OS == OPT_OS_ZEPHYR
#include <zephyr/kernel.h>
int CFG_TUSB_DEBUG_PRINTF(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printk(format, args);
    va_end(args);
    return 0;
}

#ifdef CFG_TUSB_DEBUG
  #define USBD_STACK_SIZE     (5 * CONFIG_IDLE_STACK_SIZE)
#else
  #define USBD_STACK_SIZE     (5 * CONFIG_IDLE_STACK_SIZE / 2)
#endif

struct k_thread tinyusb_thread_data;
K_THREAD_STACK_DEFINE(tinyusb_thread_stack, USBD_STACK_SIZE);
k_tid_t _tinyusb_tid;

// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
static void tinyusb_thread(void *unused0, void *unused1, void *unused2) {
    (void)unused0;
    (void)unused1;
    (void)unused2;

    // RTOS forever loop
    while (1) {
        // tinyusb device task
        if (tusb_inited()) {
            tud_task();
            tud_cdc_write_flush();
        } else {
            k_sleep(K_MSEC(1000));
        }
    }
}

#endif

bool usb_enabled(void) {
    return tusb_inited();
}

bool usb_connected(void) {
    #if R2P2_TINYUSB && R2P2_USB_DEVICE
    return tud_ready();
    #else
    return false;
    #endif
}

__attribute__((weak)) void post_usb_init(void) {
}

void usb_init(void) {
    #if R2P2_USB_DEVICE
    usb_identification_t defaults;
    usb_identification_t *identification;
    #if R2P2_USB_IDENTIFICATION
    if (custom_usb_identification != NULL) {
        identification = custom_usb_identification;
    } else {
    #endif
    // This compiles to less code than using a struct initializer.
    defaults.vid = USB_VID;
    defaults.pid = USB_PID;
    strcpy(defaults.manufacturer_name, USB_MANUFACTURER);
    strcpy(defaults.product_name, USB_PRODUCT);
    identification = &defaults;
    // This memory only needs to be live through the end of usb_build_descriptors.
    #if R2P2_USB_IDENTIFICATION
}
    #endif
    if (!usb_build_descriptors(identification)) {
        return;
    }
    init_usb_hardware();
    // Only init device. Host gets inited by the `usb_host` module common-hal.
    tud_init(TUD_OPT_RHPORT);
    #endif

    post_usb_init();

    #if MICROPY_KBD_EXCEPTION && R2P2_USB_DEVICE && R2P2_USB_CDC
    // Route Ctrl+C to the console CDC channel when the transport is active.

    // Don't watch for ctrl-C if there is no REPL.
    if (usb_cdc_console_enabled()) {
        // Console will always be itf 0.
        tud_cdc_set_wanted_char(CHAR_CTRL_C);
    }
    #endif

    #if CFG_TUSB_OS == OPT_OS_ZEPHYR
    _tinyusb_tid = k_thread_create(&tinyusb_thread_data, tinyusb_thread_stack,
        K_THREAD_STACK_SIZEOF(tinyusb_thread_stack),
        tinyusb_thread,
        NULL, NULL, NULL,
        CONFIG_MAIN_THREAD_PRIORITY - 1, 0, K_NO_WAIT);
    k_thread_name_set(_tinyusb_tid, "tinyusb");
    #endif
}

void usb_background(void) {
    if (usb_enabled()) {
        #if CFG_TUSB_OS == OPT_OS_NONE || CFG_TUSB_OS == OPT_OS_PICO
        tud_task();
        #if R2P2_USB_HOST || R2P2_MAX3421E
        tuh_task();
        #endif
        #elif CFG_TUSB_OS == OPT_OS_FREERTOS
        // TinyUSB may run in a separate task, at the same priority as the runtime.
        port_task_yield();
        #endif
        // No need to flush if there's no REPL.
        #if R2P2_USB_DEVICE && R2P2_USB_CDC
        if (usb_cdc_console_enabled()) {
            // Console will always be itf 0.
            tud_cdc_write_flush();
        }
        #endif
    }
}

uint32_t tusb_time_millis_api(void) {
    return supervisor_ticks_ms32();
}

static background_callback_t usb_callback;
static void usb_background_do(void *unused) {
    usb_background();
}

void PLACE_IN_ITCM(usb_background_schedule)(void) {
    background_callback_add(&usb_callback, usb_background_do, NULL);
}

void PLACE_IN_ITCM(usb_irq_handler)(int instance) {
    #if CFG_TUSB_MCU != OPT_MCU_RP2040
    #if R2P2_USB_DEVICE
    // For rp2040, IRQ handler is already installed and invoked automatically
    if (instance == R2P2_USB_DEVICE_INSTANCE) {
        tud_int_handler(instance);
    }
    #endif
    #if R2P2_USB_HOST
    if (instance == R2P2_USB_HOST_INSTANCE) {
        tuh_int_handler(instance);
    }
    #endif
    #endif

    usb_background_schedule();
}
