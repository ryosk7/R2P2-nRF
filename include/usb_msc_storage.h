#ifndef USB_MSC_STORAGE_H
#define USB_MSC_STORAGE_H

#include "app_usbd.h"

void usb_msc_storage_init(void);
app_usbd_class_inst_t const *usb_msc_storage_class(void);

#endif
