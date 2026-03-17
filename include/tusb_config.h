#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#include "r2p2_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#define CFG_TUD_ENABLED R2P2_USB_DEVICE

#if R2P2_USB_DEVICE_INSTANCE == 0
#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#elif R2P2_USB_DEVICE_INSTANCE == 1
#define CFG_TUSB_RHPORT1_MODE OPT_MODE_DEVICE
#endif

#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_CDC 2
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_CUSTOM_CLASS 0
#define CFG_TUH_ENABLED 0

#define CFG_TUSB_ATTR_USBRAM __attribute__((section(R2P2_TUSB_ATTR_USBRAM)))
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(R2P2_TUSB_MEM_ALIGN)))

#ifdef __cplusplus
}
#endif

#endif
