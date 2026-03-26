#include "usb_cdc_transport.h"

#include <string.h>

#include "app_usbd_cdc_acm.h"
#include "app_util_platform.h"
#include "r2p2_config.h"

extern app_usbd_cdc_acm_t const m_r2p2_console_cdc_acm;
extern app_usbd_cdc_acm_t const m_r2p2_data_cdc_acm;

typedef struct {
  bool connected;
  bool rx_pending;
  bool tx_pending;
  uint8_t ring[R2P2_USB_RING_BUFFER_SIZE];
  size_t head;
  size_t tail;
  size_t count;
  uint8_t staging[R2P2_USB_RX_BUFFER_SIZE];
} cdc_channel_state_t;

static cdc_channel_state_t cdc_channels[2];

static app_usbd_cdc_acm_t const *channel_instance(r2p2_usb_channel_t channel) {
  return channel == R2P2_USB_CHANNEL_CONSOLE ? &m_r2p2_console_cdc_acm : &m_r2p2_data_cdc_acm;
}

static cdc_channel_state_t *channel_state(r2p2_usb_channel_t channel) {
  return &cdc_channels[(size_t)channel];
}

static void ring_reset(cdc_channel_state_t *state) {
  state->head = 0;
  state->tail = 0;
  state->count = 0;
}

static void ring_push_byte(cdc_channel_state_t *state, uint8_t value) {
  if (state->count == R2P2_USB_RING_BUFFER_SIZE) {
    state->tail = (state->tail + 1) % R2P2_USB_RING_BUFFER_SIZE;
    state->count--;
  }

  state->ring[state->head] = value;
  state->head = (state->head + 1) % R2P2_USB_RING_BUFFER_SIZE;
  state->count++;
}

static int ring_pop_byte(cdc_channel_state_t *state) {
  uint8_t value;

  if (state->count == 0) {
    return -1;
  }

  value = state->ring[state->tail];
  state->tail = (state->tail + 1) % R2P2_USB_RING_BUFFER_SIZE;
  state->count--;
  return value;
}

static void consume_completed_read(r2p2_usb_channel_t channel) {
  app_usbd_cdc_acm_t const *instance = channel_instance(channel);
  cdc_channel_state_t *state = channel_state(channel);
  size_t received = app_usbd_cdc_acm_rx_size(instance);

  for (size_t i = 0; i < received; i++) {
    ring_push_byte(state, state->staging[i]);
  }
}

static void schedule_read(r2p2_usb_channel_t channel) {
  app_usbd_cdc_acm_t const *instance = channel_instance(channel);
  cdc_channel_state_t *state = channel_state(channel);
  ret_code_t ret;

  if (!state->connected || state->rx_pending) {
    return;
  }

  while (true) {
    ret = app_usbd_cdc_acm_read_any(instance, state->staging, sizeof(state->staging));
    if (ret == NRF_ERROR_IO_PENDING) {
      state->rx_pending = true;
      return;
    }
    if (ret != NRF_SUCCESS) {
      return;
    }
    consume_completed_read(channel);
  }
}

void usb_cdc_transport_init(void) {
  memset(cdc_channels, 0, sizeof(cdc_channels));
}

app_usbd_class_inst_t const *usb_cdc_transport_console_class(void) {
  return app_usbd_cdc_acm_class_inst_get(&m_r2p2_console_cdc_acm);
}

app_usbd_class_inst_t const *usb_cdc_transport_data_class(void) {
  return app_usbd_cdc_acm_class_inst_get(&m_r2p2_data_cdc_acm);
}

void usb_cdc_transport_on_port_open(r2p2_usb_channel_t channel) {
  cdc_channel_state_t *state = channel_state(channel);

  state->connected = true;
  state->rx_pending = false;
  state->tx_pending = false;
  schedule_read(channel);
}

void usb_cdc_transport_on_port_close(r2p2_usb_channel_t channel) {
  cdc_channel_state_t *state = channel_state(channel);

  state->connected = false;
  state->rx_pending = false;
  state->tx_pending = false;
  ring_reset(state);
}

void usb_cdc_transport_on_rx_done(r2p2_usb_channel_t channel) {
  cdc_channel_state_t *state = channel_state(channel);

  state->rx_pending = false;
  consume_completed_read(channel);
  schedule_read(channel);
}

void usb_cdc_transport_on_tx_done(r2p2_usb_channel_t channel) {
  channel_state(channel)->tx_pending = false;
}

bool usb_cdc_transport_channel_connected(r2p2_usb_channel_t channel) {
  return channel_state(channel)->connected;
}

bool usb_cdc_transport_any_connected(void) {
  return usb_cdc_transport_channel_connected(R2P2_USB_CHANNEL_CONSOLE) ||
    usb_cdc_transport_channel_connected(R2P2_USB_CHANNEL_DATA);
}

size_t usb_cdc_transport_write(r2p2_usb_channel_t channel, const uint8_t *data, size_t length) {
  cdc_channel_state_t *state = channel_state(channel);
  size_t written = 0;

  if (!usb_cdc_transport_channel_connected(channel)) {
    return 0;
  }

  while (written < length && state->connected) {
    size_t chunk = length - written;
    ret_code_t ret;

    if (chunk > 64) {
      chunk = 64;
    }

    while (state->tx_pending && state->connected) {
      app_usbd_event_queue_process();
    }

    ret = app_usbd_cdc_acm_write(channel_instance(channel), data + written, chunk);
    if (ret == NRF_SUCCESS) {
      state->tx_pending = true;
      written += chunk;
      continue;
    }

    if (ret == NRF_ERROR_BUSY || ret == NRF_ERROR_IO_PENDING) {
      app_usbd_event_queue_process();
      continue;
    }

    break;
  }

  while (state->tx_pending && state->connected) {
    app_usbd_event_queue_process();
  }

  return written;
}

size_t usb_cdc_transport_bytes_available(r2p2_usb_channel_t channel) {
  return channel_state(channel)->count;
}

int usb_cdc_transport_read(r2p2_usb_channel_t channel) {
  return ring_pop_byte(channel_state(channel));
}
