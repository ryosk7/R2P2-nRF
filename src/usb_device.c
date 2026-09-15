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

/*
 * Ask the bootloader for UF2 mode on the next boot.
 *
 * 0x57 is DFU_MAGIC_UF2_RESET (Adafruit bootloader, src/main.c:109). It
 * takes the usb_init() branch and enumerates CDC+MSC, which is the
 * drag-and-drop route this call exists to reach. The OTA magics -- 0xA8
 * DFU_MAGIC_OTA_RESET and 0xB1 DFU_MAGIC_OTA_APPJUM -- instead take
 *
 *     if (_ota_dfu) { if (!_sd_inited) mbr_init_sd(); ble_stack_init(); }
 *
 * and wait for a host speaking the BLE DFU protocol. Nothing here can
 * satisfy that. 0x4e (serial only) and 0x6d (skip) are the remaining
 * magics and are not what is wanted either.
 *
 * Worth knowing about the path we do take: uf2_dfu reaches
 * bootloader_dfu_start(_ota_dfu, 3000, true), so if USB does not
 * enumerate within three seconds -- a board on battery, say -- the
 * bootloader restarts into the application rather than sitting in DFU.
 *
 * GPREGRET is written directly: the firmware links nrf_soc_nosd and
 * never enables a SoftDevice, so sd_power_gpregret_set does not apply.
 *
 * Note the double-reset cell the bootloader uses for the two-tap route
 * is a different mechanism and is not reachable from here -- it requires
 * RESETREAS.RESETPIN (src/main.c:255), which a software reset does not
 * set.
 */
#define R2P2_DFU_MAGIC_UF2_RESET 0x57

__attribute__((weak)) void r2p2_reset_to_bootloader(void) {
  NRF_POWER->GPREGRET = R2P2_DFU_MAGIC_UF2_RESET;
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
