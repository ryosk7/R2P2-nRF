#include "usb_msc_storage.h"

#include "app_usbd_msc.h"
#include "nrf_block_dev.h"
#include "nrf_nvmc.h"
#include "r2p2_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  MSC_INTERFACE = R2P2_USB_CDC_DATA_ENABLED_DEFAULT ? 4 : 2,
  MSC_BLOCK_SIZE = 512,
  MSC_BLOCK_COUNT = 256,
  MSC_WORKBUFFER_SIZE = 1024,
  MSC_FLASH_START = 0x000B4000,
  MSC_FLASH_SIZE = MSC_BLOCK_SIZE * MSC_BLOCK_COUNT,
  MSC_FLASH_PAGE_SIZE = 4096,
  MSC_FLASH_PAGE_WORDS = MSC_FLASH_PAGE_SIZE / sizeof(uint32_t),
  MSC_FLASH_PAGE_COUNT = MSC_FLASH_SIZE / MSC_FLASH_PAGE_SIZE,
  FAT_RESERVED_SECTORS = 1,
  FAT_COUNT = 2,
  FAT_SECTORS = 2,
  FAT_ROOT_ENTRY_COUNT = 64,
  FAT_ROOT_DIR_SECTORS =
    (FAT_ROOT_ENTRY_COUNT * 32 + MSC_BLOCK_SIZE - 1) / MSC_BLOCK_SIZE,
  FAT_FIRST_DATA_SECTOR =
    FAT_RESERVED_SECTORS + FAT_COUNT * FAT_SECTORS + FAT_ROOT_DIR_SECTORS,
  FAT_BOOT_SIGNATURE_OFFSET = 510,
  FAT_ROOT_DIR_OFFSET =
    (FAT_RESERVED_SECTORS + FAT_COUNT * FAT_SECTORS) * MSC_BLOCK_SIZE,
  FAT_DATA_OFFSET = FAT_FIRST_DATA_SECTOR * MSC_BLOCK_SIZE,
};

typedef struct {
  nrf_block_dev_t block_dev;
  nrf_block_dev_info_strings_t info_strings;
  nrf_block_dev_geometry_t geometry;
  nrf_block_dev_ev_handler ev_handler;
  void const *p_context;
  bool initialized;
} r2p2_flash_block_dev_t;

static ret_code_t flash_block_dev_init(nrf_block_dev_t const *p_blk_dev,
  nrf_block_dev_ev_handler ev_handler, void const *p_context);
static ret_code_t flash_block_dev_uninit(nrf_block_dev_t const *p_blk_dev);
static ret_code_t flash_block_dev_read_req(nrf_block_dev_t const *p_blk_dev,
  nrf_block_req_t const *p_blk);
static ret_code_t flash_block_dev_write_req(nrf_block_dev_t const *p_blk_dev,
  nrf_block_req_t const *p_blk);
static ret_code_t flash_block_dev_ioctl(nrf_block_dev_t const *p_blk_dev,
  nrf_block_dev_ioctl_req_t req, void *p_data);
static nrf_block_dev_geometry_t const *flash_block_dev_geometry(
  nrf_block_dev_t const *p_blk_dev);

static const nrf_block_dev_ops_t m_flash_block_dev_ops = {
  .init = flash_block_dev_init,
  .uninit = flash_block_dev_uninit,
  .read_req = flash_block_dev_read_req,
  .write_req = flash_block_dev_write_req,
  .ioctl = flash_block_dev_ioctl,
  .geometry = flash_block_dev_geometry,
};

static r2p2_flash_block_dev_t m_r2p2_msc_flash = {
  .block_dev = { .p_ops = &m_flash_block_dev_ops },
  .info_strings = {
    .p_vendor = "R2P2",
    .p_product = "nRF52 MSC",
    .p_revision = "1.00",
  },
};

static uint32_t m_flash_page_buffer[MSC_FLASH_PAGE_WORDS];

static void msc_user_ev_handler(app_usbd_class_inst_t const *instance,
  app_usbd_msc_user_event_t event) {
  (void)instance;
  (void)event;
}

APP_USBD_MSC_GLOBAL_DEF(m_r2p2_msc,
  MSC_INTERFACE,
  msc_user_ev_handler,
  APP_USBD_MSC_ENDPOINT_LIST(5, 5),
  (&m_r2p2_msc_flash.block_dev),
  MSC_WORKBUFFER_SIZE);

static r2p2_flash_block_dev_t *flash_block_dev_from_base(
  nrf_block_dev_t const *p_blk_dev) {
  return CONTAINER_OF(p_blk_dev, r2p2_flash_block_dev_t, block_dev);
}

static uint8_t *flash_page_bytes(void) {
  return (uint8_t *)m_flash_page_buffer;
}

static uint32_t flash_offset_to_addr(uint32_t offset) {
  return MSC_FLASH_START + offset;
}

static void store_u16_le(uint8_t *dest, uint16_t value) {
  dest[0] = (uint8_t)(value & 0xffu);
  dest[1] = (uint8_t)((value >> 8) & 0xffu);
}

static void store_u32_le(uint8_t *dest, uint32_t value) {
  dest[0] = (uint8_t)(value & 0xffu);
  dest[1] = (uint8_t)((value >> 8) & 0xffu);
  dest[2] = (uint8_t)((value >> 16) & 0xffu);
  dest[3] = (uint8_t)((value >> 24) & 0xffu);
}

static bool flash_boot_sector_valid(void) {
  uint8_t const *boot = (uint8_t const *)MSC_FLASH_START;

  return boot[FAT_BOOT_SIGNATURE_OFFSET] == 0x55 &&
    boot[FAT_BOOT_SIGNATURE_OFFSET + 1] == 0xaa &&
    boot[11] == 0x00 &&
    boot[12] == 0x02 &&
    memcmp(&boot[54], "FAT12   ", 8) == 0;
}

static void fat12_set_entry(uint8_t *fat, uint16_t cluster, uint16_t value) {
  uint32_t offset = cluster + cluster / 2;

  if ((cluster & 1u) == 0) {
    fat[offset] = (uint8_t)(value & 0xffu);
    fat[offset + 1] = (uint8_t)((fat[offset + 1] & 0xf0u) |
      ((value >> 8) & 0x0fu));
  } else {
    fat[offset] = (uint8_t)((fat[offset] & 0x0fu) | ((value << 4) & 0xf0u));
    fat[offset + 1] = (uint8_t)((value >> 4) & 0xffu);
  }
}

static void flash_erase_all(void) {
  uint32_t page_addr = MSC_FLASH_START;

  for (uint32_t page = 0; page < MSC_FLASH_PAGE_COUNT; page++) {
    nrf_nvmc_page_erase(page_addr);
    page_addr += MSC_FLASH_PAGE_SIZE;
  }
}

static void flash_write_bytes(uint32_t offset, void const *data, size_t length) {
  nrf_nvmc_write_bytes(flash_offset_to_addr(offset), data, length);
}

static ret_code_t flash_update_range(uint32_t offset, void const *data,
  size_t length) {
  uint8_t const *src = (uint8_t const *)data;

  if (offset + length > MSC_FLASH_SIZE) {
    return NRF_ERROR_INVALID_ADDR;
  }

  while (length > 0) {
    uint32_t page_index = offset / MSC_FLASH_PAGE_SIZE;
    uint32_t page_offset = offset % MSC_FLASH_PAGE_SIZE;
    uint32_t chunk = MSC_FLASH_PAGE_SIZE - page_offset;
    uint32_t page_addr = MSC_FLASH_START + page_index * MSC_FLASH_PAGE_SIZE;

    if (chunk > length) {
      chunk = (uint32_t)length;
    }

    memcpy(flash_page_bytes(), (void const *)page_addr, MSC_FLASH_PAGE_SIZE);
    if (memcmp(flash_page_bytes() + page_offset, src, chunk) != 0) {
      memcpy(flash_page_bytes() + page_offset, src, chunk);
      nrf_nvmc_page_erase(page_addr);
      nrf_nvmc_write_bytes(page_addr, flash_page_bytes(), MSC_FLASH_PAGE_SIZE);
    }

    offset += chunk;
    src += chunk;
    length -= chunk;
  }

  return NRF_SUCCESS;
}

static void flash_format_volume(void) {
  static const char readme_text[] =
    "R2P2 nRF52 MSC volume\r\n"
    "PicoRuby runtime storage bring-up.\r\n";
  uint8_t sector[MSC_BLOCK_SIZE];
  uint8_t fat[FAT_SECTORS * MSC_BLOCK_SIZE];
  uint8_t root_dir[FAT_ROOT_DIR_SECTORS * MSC_BLOCK_SIZE];
  uint32_t readme_size = (uint32_t)(sizeof(readme_text) - 1);
  uint32_t cluster_offset = FAT_DATA_OFFSET;

  flash_erase_all();

  memset(sector, 0, sizeof(sector));
  sector[0] = 0xeb;
  sector[1] = 0x3c;
  sector[2] = 0x90;
  memcpy(&sector[3], "MSDOS5.0", 8);
  store_u16_le(&sector[11], MSC_BLOCK_SIZE);
  sector[13] = 1;
  store_u16_le(&sector[14], FAT_RESERVED_SECTORS);
  sector[16] = FAT_COUNT;
  store_u16_le(&sector[17], FAT_ROOT_ENTRY_COUNT);
  store_u16_le(&sector[19], MSC_BLOCK_COUNT);
  sector[21] = 0xf8;
  store_u16_le(&sector[22], FAT_SECTORS);
  store_u16_le(&sector[24], 32);
  store_u16_le(&sector[26], 1);
  store_u32_le(&sector[28], 0);
  store_u32_le(&sector[32], 0);
  sector[36] = 0x80;
  sector[38] = 0x29;
  store_u32_le(&sector[39], 0x52325032u);
  memcpy(&sector[43], "R2P2 NRF52 ", 11);
  memcpy(&sector[54], "FAT12   ", 8);
  sector[FAT_BOOT_SIGNATURE_OFFSET] = 0x55;
  sector[FAT_BOOT_SIGNATURE_OFFSET + 1] = 0xaa;
  flash_write_bytes(0, sector, sizeof(sector));

  memset(fat, 0, sizeof(fat));
  fat[0] = 0xf8;
  fat[1] = 0xff;
  fat[2] = 0xff;
  fat12_set_entry(fat, 2, 0xfffu);
  flash_write_bytes(MSC_BLOCK_SIZE * FAT_RESERVED_SECTORS, fat, sizeof(fat));
  flash_write_bytes(MSC_BLOCK_SIZE * (FAT_RESERVED_SECTORS + FAT_SECTORS), fat,
    sizeof(fat));

  memset(root_dir, 0, sizeof(root_dir));
  memcpy(&root_dir[0], "R2P2 NRF52 ", 11);
  root_dir[11] = 0x08;
  memcpy(&root_dir[32], "README  TXT", 11);
  root_dir[32 + 11] = 0x20;
  store_u16_le(&root_dir[32 + 26], 2);
  store_u32_le(&root_dir[32 + 28], readme_size);
  flash_write_bytes(FAT_ROOT_DIR_OFFSET, root_dir, sizeof(root_dir));

  memset(sector, 0, sizeof(sector));
  memcpy(sector, readme_text, readme_size);
  flash_write_bytes(cluster_offset, sector, sizeof(sector));
}

static void flash_send_event(r2p2_flash_block_dev_t const *device,
  nrf_block_dev_event_type_t ev_type, nrf_block_req_t const *req) {
  nrf_block_dev_event_t event = {
    .ev_type = ev_type,
    .result = NRF_BLOCK_DEV_RESULT_SUCCESS,
    .p_blk_req = req,
    .p_context = device->p_context,
  };

  if (device->ev_handler) {
    device->ev_handler(&device->block_dev, &event);
  }
}

static ret_code_t flash_validate_req(nrf_block_req_t const *p_blk) {
  if (p_blk == NULL || p_blk->p_buff == NULL || p_blk->blk_count == 0) {
    return NRF_ERROR_INVALID_PARAM;
  }

  if (p_blk->blk_id >= MSC_BLOCK_COUNT ||
      p_blk->blk_count > (MSC_BLOCK_COUNT - p_blk->blk_id)) {
    return NRF_ERROR_INVALID_ADDR;
  }

  return NRF_SUCCESS;
}

static ret_code_t flash_block_dev_init(nrf_block_dev_t const *p_blk_dev,
  nrf_block_dev_ev_handler ev_handler, void const *p_context) {
  r2p2_flash_block_dev_t *device = flash_block_dev_from_base(p_blk_dev);

  device->geometry.blk_size = MSC_BLOCK_SIZE;
  device->geometry.blk_count = MSC_BLOCK_COUNT;
  device->ev_handler = ev_handler;
  device->p_context = p_context;
  device->initialized = true;

  if (!flash_boot_sector_valid()) {
    flash_format_volume();
  }

  flash_send_event(device, NRF_BLOCK_DEV_EVT_INIT, NULL);
  return NRF_SUCCESS;
}

static ret_code_t flash_block_dev_uninit(nrf_block_dev_t const *p_blk_dev) {
  r2p2_flash_block_dev_t *device = flash_block_dev_from_base(p_blk_dev);

  device->initialized = false;
  flash_send_event(device, NRF_BLOCK_DEV_EVT_UNINIT, NULL);
  return NRF_SUCCESS;
}

static ret_code_t flash_block_dev_read_req(nrf_block_dev_t const *p_blk_dev,
  nrf_block_req_t const *p_blk) {
  r2p2_flash_block_dev_t *device = flash_block_dev_from_base(p_blk_dev);
  ret_code_t ret = flash_validate_req(p_blk);
  uint32_t offset;

  if (ret != NRF_SUCCESS) {
    return ret;
  }

  offset = p_blk->blk_id * MSC_BLOCK_SIZE;
  memcpy(p_blk->p_buff, (void const *)flash_offset_to_addr(offset),
    p_blk->blk_count * MSC_BLOCK_SIZE);
  flash_send_event(device, NRF_BLOCK_DEV_EVT_BLK_READ_DONE, p_blk);
  return NRF_SUCCESS;
}

static ret_code_t flash_block_dev_write_req(nrf_block_dev_t const *p_blk_dev,
  nrf_block_req_t const *p_blk) {
  r2p2_flash_block_dev_t *device = flash_block_dev_from_base(p_blk_dev);
  ret_code_t ret = flash_validate_req(p_blk);
  uint32_t offset;

  if (ret != NRF_SUCCESS) {
    return ret;
  }

  offset = p_blk->blk_id * MSC_BLOCK_SIZE;
  ret = flash_update_range(offset, p_blk->p_buff,
    p_blk->blk_count * MSC_BLOCK_SIZE);
  if (ret != NRF_SUCCESS) {
    return ret;
  }

  flash_send_event(device, NRF_BLOCK_DEV_EVT_BLK_WRITE_DONE, p_blk);
  return NRF_SUCCESS;
}

static ret_code_t flash_block_dev_ioctl(nrf_block_dev_t const *p_blk_dev,
  nrf_block_dev_ioctl_req_t req, void *p_data) {
  r2p2_flash_block_dev_t *device = flash_block_dev_from_base(p_blk_dev);

  switch (req) {
    case NRF_BLOCK_DEV_IOCTL_REQ_CACHE_FLUSH:
      if (p_data != NULL) {
        *(bool *)p_data = false;
      }
      return NRF_SUCCESS;

    case NRF_BLOCK_DEV_IOCTL_REQ_INFO_STRINGS:
      if (p_data == NULL) {
        return NRF_ERROR_INVALID_PARAM;
      }
      *(nrf_block_dev_info_strings_t const **)p_data = &device->info_strings;
      return NRF_SUCCESS;

    default:
      return NRF_ERROR_NOT_SUPPORTED;
  }
}

static nrf_block_dev_geometry_t const *flash_block_dev_geometry(
  nrf_block_dev_t const *p_blk_dev) {
  return &flash_block_dev_from_base(p_blk_dev)->geometry;
}

void usb_msc_storage_init(void) {
}

app_usbd_class_inst_t const *usb_msc_storage_class(void) {
  return app_usbd_msc_class_inst_get(&m_r2p2_msc);
}
