#ifndef USB_CDC_TRANSPORT_H
#define USB_CDC_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "r2p2_nrf52_usb.h"

void usb_cdc_transport_init(void);
app_usbd_class_inst_t const *usb_cdc_transport_console_class(void);
app_usbd_class_inst_t const *usb_cdc_transport_data_class(void);
void usb_cdc_transport_on_port_open(r2p2_usb_channel_t channel);
void usb_cdc_transport_on_port_close(r2p2_usb_channel_t channel);
void usb_cdc_transport_on_rx_done(r2p2_usb_channel_t channel);
void usb_cdc_transport_on_tx_done(r2p2_usb_channel_t channel);
bool usb_cdc_transport_channel_connected(r2p2_usb_channel_t channel);
bool usb_cdc_transport_any_connected(void);
size_t usb_cdc_transport_write(r2p2_usb_channel_t channel, const uint8_t *data, size_t length);
size_t usb_cdc_transport_bytes_available(r2p2_usb_channel_t channel);
int usb_cdc_transport_read(r2p2_usb_channel_t channel);

#endif
