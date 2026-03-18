BOARD ?= ssci_isp1807_dev_board

ROOT := $(abspath .)
BUILD_DIR := $(ROOT)/build/$(BOARD)
SRC_DIR := $(ROOT)/src
INCLUDE_DIR := $(ROOT)/include
BUILD_CONFIG_DIR := $(ROOT)/build_config
COMPONENTS_DIR := $(ROOT)/components
PICORUBY_NRF52_DIR := $(COMPONENTS_DIR)/picoruby-nRF52
PICORUBY_NRF52_ROOT ?= $(abspath $(ROOT)/../picoruby-nRF52)
GNU_PREFIX ?= arm-none-eabi
empty :=
space := $(empty) $(empty)

CC := $(GNU_PREFIX)-gcc
OBJCOPY := $(GNU_PREFIX)-objcopy
SIZE := $(GNU_PREFIX)-size

include $(PICORUBY_NRF52_ROOT)/build_config/nrf52-sdk.mk
include $(BUILD_CONFIG_DIR)/$(BOARD).mk

OBJ_DIR := $(BUILD_DIR)/obj
FIRMWARE_OUT := $(BUILD_DIR)/firmware.out
FIRMWARE_HEX := $(BUILD_DIR)/firmware.hex
FIRMWARE_BIN := $(BUILD_DIR)/firmware.bin
FIRMWARE_UF2 := $(BUILD_DIR)/firmware.uf2
SDK_ROOT := $(NRF5_SDK_ROOT)
LINKER_SCRIPT := $(NRF52_LINKER_SCRIPT)
SDK_CONFIG_DIR := $(NRF52_SDK_CONFIG_DIR)
STARTUP_SRC := $(NRF52_STARTUP_SRC)

SRC_FILES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/serial_transport.c \
	$(SRC_DIR)/usb_cdc_transport.c \
	$(SRC_DIR)/usb_device.c \
	$(SRC_DIR)/usb_runtime.c \
	$(STARTUP_SRC) \
	$(SDK_ROOT)/components/libraries/util/app_error.c \
	$(SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
	$(SDK_ROOT)/components/libraries/util/app_error_weak.c \
	$(SDK_ROOT)/components/libraries/timer/app_timer2.c \
	$(SDK_ROOT)/components/libraries/timer/drv_rtc.c \
	$(SDK_ROOT)/components/libraries/usbd/app_usbd.c \
	$(SDK_ROOT)/components/libraries/usbd/app_usbd_core.c \
	$(SDK_ROOT)/components/libraries/usbd/app_usbd_serial_num.c \
	$(SDK_ROOT)/components/libraries/usbd/app_usbd_string_desc.c \
	$(SDK_ROOT)/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c \
	$(SDK_ROOT)/components/libraries/util/app_util_platform.c \
	$(SDK_ROOT)/components/libraries/hardfault/nrf52/handler/hardfault_handler_gcc.c \
	$(SDK_ROOT)/components/libraries/hardfault/hardfault_implementation.c \
	$(SDK_ROOT)/components/libraries/util/nrf_assert.c \
	$(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
	$(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
	$(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
	$(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
	$(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
	$(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
	$(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
	$(SDK_ROOT)/components/libraries/sortlist/nrf_sortlist.c \
	$(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
	$(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
	$(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_power.c \
	$(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd/nrf_nvic.c \
	$(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd/nrf_soc.c \
	$(SDK_ROOT)/modules/nrfx/soc/nrfx_atomic.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_systick.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_usbd.c \
	$(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c \
	$(SDK_ROOT)/external/utf_converter/utf.c

INC_DIRS := \
	$(INCLUDE_DIR) \
	$(SDK_CONFIG_DIR) \
	$(SDK_ROOT)/components \
	$(SDK_ROOT)/components/libraries/timer \
	$(SDK_ROOT)/components/libraries/util \
	$(SDK_ROOT)/components/libraries/log \
	$(SDK_ROOT)/components/libraries/log/src \
	$(SDK_ROOT)/components/libraries/usbd \
	$(SDK_ROOT)/components/libraries/usbd/class/cdc \
	$(SDK_ROOT)/components/libraries/usbd/class/cdc/acm \
	$(SDK_ROOT)/components/libraries/balloc \
	$(SDK_ROOT)/components/libraries/ringbuf \
	$(SDK_ROOT)/components/libraries/hardfault/nrf52 \
	$(SDK_ROOT)/components/libraries/hardfault \
	$(SDK_ROOT)/components/libraries/atomic_fifo \
	$(SDK_ROOT)/components/libraries/atomic \
	$(SDK_ROOT)/components/libraries/memobj \
	$(SDK_ROOT)/components/libraries/queue \
	$(SDK_ROOT)/components/libraries/experimental_section_vars \
	$(SDK_ROOT)/components/libraries/sortlist \
	$(SDK_ROOT)/components/libraries/strerror \
	$(SDK_ROOT)/components/libraries/delay \
	$(SDK_ROOT)/components/toolchain/cmsis/include \
	$(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd \
	$(SDK_ROOT)/integration/nrfx \
	$(SDK_ROOT)/integration/nrfx/legacy \
	$(SDK_ROOT)/modules/nrfx \
	$(SDK_ROOT)/modules/nrfx/drivers/include \
	$(SDK_ROOT)/modules/nrfx/hal \
	$(SDK_ROOT)/modules/nrfx/mdk \
	$(SDK_ROOT)/external/utf_converter

CPPFLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS += \
	-DR2P2_USB_VENDOR_ID=$(R2P2_USB_VENDOR_ID) \
	-DR2P2_USB_PRODUCT_ID=$(R2P2_USB_PRODUCT_ID) \
	-DR2P2_USB_MANUFACTURER=\"$(subst $(space),\$(space),$(R2P2_USB_MANUFACTURER))\" \
	-DR2P2_USB_PRODUCT=\"$(subst $(space),\$(space),$(R2P2_USB_PRODUCT))\" \
	-DR2P2_USB_CONFIGURATION=\"$(subst $(space),\$(space),$(R2P2_USB_CONFIGURATION))\" \
	-DR2P2_USB_CDC_CONSOLE_ENABLED_DEFAULT=$(R2P2_USB_CDC_CONSOLE_ENABLED_DEFAULT) \
	-DR2P2_USB_CDC_DATA_ENABLED_DEFAULT=$(R2P2_USB_CDC_DATA_ENABLED_DEFAULT) \
	-DR2P2_USB_MSC=$(R2P2_USB_MSC) \
	-DR2P2_USB_HID=$(R2P2_USB_HID) \
	-DR2P2_USB_MIDI=$(R2P2_USB_MIDI)
CPPFLAGS += \
	-DNRF52840_XXAA \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DAPP_TIMER_V2 \
	-DAPP_TIMER_V2_RTC1_ENABLED

CFLAGS := \
	-std=gnu11 \
	-O2 \
	-g3 \
	-mcpu=cortex-m4 \
	-mthumb \
	-mabi=aapcs \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-Wno-expansion-to-defined \
	-Wno-array-bounds \
	-ffunction-sections \
	-fdata-sections \
	-fno-strict-aliasing \
	-fno-builtin \
	-fshort-enums

LDFLAGS := \
	-mcpu=cortex-m4 \
	-mthumb \
	-mabi=aapcs \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-Wl,--gc-sections \
	-specs=nano.specs \
	-L$(SDK_ROOT)/modules/nrfx/mdk \
	-T$(LINKER_SCRIPT)

LDLIBS := -lc -lnosys -lm

define object_path
$(OBJ_DIR)/$(subst /,_,$(basename $(1))).o
endef

OBJECTS := $(foreach src,$(SRC_FILES),$(call object_path,$(src)))
DEPS := $(OBJECTS:.o=.d)

.PHONY: build-cdc-dual clean

build-cdc-dual: $(FIRMWARE_UF2)

ifeq ($(wildcard $(SDK_ROOT)/components/toolchain/gcc/Makefile.common),)
$(error nRF5 SDK not found at $(SDK_ROOT). Place $(NRF5_SDK_VERSION) under $(PICORUBY_NRF52_ROOT)/nrf52/sdk/)
endif



$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

define compile_rule
$(call object_path,$(1)): $(1) | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $$< -o $$@
endef

$(foreach src,$(SRC_FILES),$(eval $(call compile_rule,$(src))))

$(FIRMWARE_OUT): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(FIRMWARE_HEX): $(FIRMWARE_OUT)
	$(OBJCOPY) -O ihex $< $@

$(FIRMWARE_BIN): $(FIRMWARE_OUT)
	$(OBJCOPY) -O binary $< $@

$(FIRMWARE_UF2): $(FIRMWARE_HEX)
	ruby $(ROOT)/tools/uf2conv.rb -c -f $(R2P2_UF2_FAMILY) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
