#include "usb_msc_storage.h"

#include "app_usbd_msc.h"
#include "nrf_block_dev_empty.h"
#include "r2p2_config.h"

enum {
  MSC_INTERFACE = R2P2_USB_CDC_DATA_ENABLED_DEFAULT ? 4 : 2,
  MSC_BLOCK_SIZE = 512,
  MSC_BLOCK_COUNT = 1024,
  MSC_WORKBUFFER_SIZE = 1024,
};

static void msc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_msc_user_event_t event) {
  (void)instance;
  (void)event;
}

NRF_BLOCK_DEV_EMPTY_DEFINE(m_r2p2_msc_empty,
  NRF_BLOCK_DEV_EMPTY_CONFIG(MSC_BLOCK_SIZE, MSC_BLOCK_COUNT),
  NFR_BLOCK_DEV_INFO_CONFIG("R2P2", "nRF52 MSC", "1.00"));

APP_USBD_MSC_GLOBAL_DEF(m_r2p2_msc,
  MSC_INTERFACE,
  msc_user_ev_handler,
  APP_USBD_MSC_ENDPOINT_LIST(5, 5),
  (NRF_BLOCKDEV_BASE_ADDR(m_r2p2_msc_empty, block_dev)),
  MSC_WORKBUFFER_SIZE);

void usb_msc_storage_init(void) {
}

app_usbd_class_inst_t const *usb_msc_storage_class(void) {
  return app_usbd_msc_class_inst_get(&m_r2p2_msc);
}
