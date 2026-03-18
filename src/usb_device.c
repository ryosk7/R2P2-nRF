#include "r2p2_nrf52_usb.h"

#include "app_usbd_cdc_acm.h"
#include "nrf.h"
#include "usb_cdc_transport.h"

enum {
  CDC_CONSOLE_COMM_INTERFACE = 0,
  CDC_CONSOLE_DATA_INTERFACE = 1,
  CDC_DATA_COMM_INTERFACE = 2,
  CDC_DATA_DATA_INTERFACE = 3,
};

void console_cdc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_cdc_acm_user_event_t event);
void data_cdc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_cdc_acm_user_event_t event);

APP_USBD_CDC_ACM_GLOBAL_DEF(m_r2p2_console_cdc_acm,
  console_cdc_user_ev_handler,
  CDC_CONSOLE_COMM_INTERFACE,
  CDC_CONSOLE_DATA_INTERFACE,
  NRF_DRV_USBD_EPIN2,
  NRF_DRV_USBD_EPIN1,
  NRF_DRV_USBD_EPOUT1,
  APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

APP_USBD_CDC_ACM_GLOBAL_DEF(m_r2p2_data_cdc_acm,
  data_cdc_user_ev_handler,
  CDC_DATA_COMM_INTERFACE,
  CDC_DATA_DATA_INTERFACE,
  NRF_DRV_USBD_EPIN4,
  NRF_DRV_USBD_EPIN3,
  NRF_DRV_USBD_EPOUT3,
  APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

__attribute__((weak)) void r2p2_reset_to_bootloader(void) {
  NVIC_SystemReset();
}

__attribute__((weak)) void r2p2_wake_main_task(void) {
}

static void cdc_user_event_handler(r2p2_usb_channel_t channel,
  app_usbd_class_inst_t const *instance,
  app_usbd_cdc_acm_user_event_t event) {
  (void)instance;

  switch (event) {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
      usb_cdc_transport_on_port_open(channel);
      break;

    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
      usb_cdc_transport_on_port_close(channel);
      break;

    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
      usb_cdc_transport_on_rx_done(channel);
      r2p2_wake_main_task();
      break;

    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
      usb_cdc_transport_on_tx_done(channel);
      break;
  }
}

void console_cdc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_cdc_acm_user_event_t event) {
  cdc_user_event_handler(R2P2_USB_CHANNEL_CONSOLE, instance, event);
}

void data_cdc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_cdc_acm_user_event_t event) {
  cdc_user_event_handler(R2P2_USB_CHANNEL_DATA, instance, event);
}
