#include "r2p2_nrf52_usb.h"

#include "app_error.h"
#include "app_timer.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_serial_num.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "r2p2_config.h"
#include "serial_transport.h"
#include "usb_cdc_transport.h"
#include "usb_msc_storage.h"

static void usbd_user_ev_handler(app_usbd_event_type_t event) {
  switch (event) {
    case APP_USBD_EVT_POWER_DETECTED:
      if (!nrf_drv_usbd_is_enabled()) {
        app_usbd_enable();
      }
      break;

    case APP_USBD_EVT_POWER_READY:
      app_usbd_start();
      break;

    case APP_USBD_EVT_POWER_REMOVED:
      app_usbd_stop();
      break;

    case APP_USBD_EVT_STOPPED:
      app_usbd_disable();
      break;

    default:
      break;
  }
}

bool r2p2_usb_connected(void) {
  return usb_cdc_transport_any_connected();
}

void r2p2_usb_init(void) {
  ret_code_t ret;
  static const app_usbd_config_t usbd_config = {
    .ev_state_proc = usbd_user_ev_handler,
    .enable_sof = false,
  };

  ret = nrf_drv_clock_init();
  if (ret != NRF_SUCCESS && ret != NRF_ERROR_MODULE_ALREADY_INITIALIZED) {
    APP_ERROR_CHECK(ret);
  }

  nrf_drv_clock_lfclk_request(NULL);
  while (!nrf_drv_clock_lfclk_is_running()) {
  }

  ret = app_timer_init();
  if (ret != NRF_SUCCESS && ret != NRF_ERROR_MODULE_ALREADY_INITIALIZED) {
    APP_ERROR_CHECK(ret);
  }

  ret = nrf_drv_power_init(NULL);
  if (ret != NRF_SUCCESS && ret != NRF_ERROR_MODULE_ALREADY_INITIALIZED) {
    APP_ERROR_CHECK(ret);
  }

  usb_cdc_transport_init();
  usb_msc_storage_init();
  app_usbd_serial_num_generate();

  ret = app_usbd_init(&usbd_config);
  APP_ERROR_CHECK(ret);

  ret = app_usbd_class_append(usb_cdc_transport_console_class());
  APP_ERROR_CHECK(ret);

  if (R2P2_USB_CDC_DATA_ENABLED_DEFAULT) {
    ret = app_usbd_class_append(usb_cdc_transport_data_class());
    APP_ERROR_CHECK(ret);
  }

  if (R2P2_USB_MSC) {
    ret = app_usbd_class_append(usb_msc_storage_class());
    APP_ERROR_CHECK(ret);
  }

  ret = app_usbd_power_events_enable();
  APP_ERROR_CHECK(ret);

  switch (nrf_drv_power_usbstatus_get()) {
    case NRF_DRV_POWER_USB_STATE_CONNECTED:
      if (!nrf_drv_usbd_is_enabled()) {
        app_usbd_enable();
      }
      break;
    case NRF_DRV_POWER_USB_STATE_READY:
      if (!nrf_drv_usbd_is_enabled()) {
        app_usbd_enable();
      }
      app_usbd_start();
      break;
    case NRF_DRV_POWER_USB_STATE_DISCONNECTED:
    default:
      break;
  }

  serial_early_init();
  serial_init();
}

void r2p2_usb_task(void) {
  while (app_usbd_event_queue_process()) {
  }
}

bool r2p2_usb_channel_connected(r2p2_usb_channel_t channel) {
  return usb_cdc_transport_channel_connected(channel);
}

size_t r2p2_usb_write(r2p2_usb_channel_t channel, const uint8_t *data, size_t length) {
  return usb_cdc_transport_write(channel, data, length);
}

size_t r2p2_usb_bytes_available(r2p2_usb_channel_t channel) {
  return usb_cdc_transport_bytes_available(channel);
}

int r2p2_usb_read(r2p2_usb_channel_t channel) {
  return usb_cdc_transport_read(channel);
}

void r2p2_usb_disconnect(void) {
  app_usbd_stop();
  app_usbd_disable();
}
