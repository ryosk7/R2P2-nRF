BOARD ?= ssci_isp1807_dev_board
CP_VERSION ?= 10.1.4

ROOT := $(abspath .)
PORTS_DIR := $(ROOT)/ports/nrf52
VENDOR_ROOT := $(PORTS_DIR)/vendor
BUILD_ROOT := $(ROOT)/build
BUILD_DIR := $(BUILD_ROOT)/$(BOARD)
RUNTIME_ROOT := $(BUILD_ROOT)/runtime-src

ROOT_RUNTIME_NORDIC_MAKEFILE := $(PORTS_DIR)/runtime/ports/nordic/Makefile
ROOT_RUNTIME_SUPERVISOR_MK := $(PORTS_DIR)/runtime/supervisor/supervisor.mk
ROOT_RUNTIME_USB_CDC_INIT := $(PORTS_DIR)/runtime/shared-module/usb_cdc/__init__.c
ROOT_RUNTIME_USB_CDC_MODULE_H := $(PORTS_DIR)/runtime/shared-module/usb_cdc/__init__.h
ROOT_RUNTIME_USB_C := $(PORTS_DIR)/runtime/supervisor/shared/usb/usb.c
ROOT_RUNTIME_USB_DESC_C := $(PORTS_DIR)/runtime/supervisor/shared/usb/usb_desc.c
ROOT_RUNTIME_USB_DEVICE_C := $(PORTS_DIR)/runtime/supervisor/shared/usb/usb_device.c
ROOT_RUNTIME_TUSB_CONFIG_H := $(PORTS_DIR)/runtime/supervisor/shared/usb/tusb_config.h
ROOT_RUNTIME_SERIAL_C := $(PORTS_DIR)/runtime/supervisor/shared/serial.c
ROOT_RUNTIME_SERIAL_H := $(PORTS_DIR)/runtime/supervisor/shared/serial.h
ROOT_RUNTIME_CONFIG_H := $(PORTS_DIR)/runtime/supervisor/shared/r2p2_runtime_config.h
BOARD_OVERRIDE_MK := $(PORTS_DIR)/boards/$(BOARD)/mpconfigboard.mk

RUNTIME_NORDIC_MAKEFILE := $(RUNTIME_ROOT)/ports/nordic/Makefile
RUNTIME_SUPERVISOR_MK := $(RUNTIME_ROOT)/supervisor/supervisor.mk
RUNTIME_USB_CDC_INIT := $(RUNTIME_ROOT)/shared-module/usb_cdc/__init__.c
RUNTIME_USB_CDC_MODULE_H := $(RUNTIME_ROOT)/shared-module/usb_cdc/__init__.h
RUNTIME_USB_C := $(RUNTIME_ROOT)/supervisor/shared/usb/usb.c
RUNTIME_USB_DESC_C := $(RUNTIME_ROOT)/supervisor/shared/usb/usb_desc.c
RUNTIME_USB_DEVICE_C := $(RUNTIME_ROOT)/supervisor/shared/usb/usb_device.c
RUNTIME_TUSB_CONFIG_H := $(RUNTIME_ROOT)/supervisor/shared/usb/tusb_config.h
RUNTIME_SERIAL_C := $(RUNTIME_ROOT)/supervisor/shared/serial.c
RUNTIME_SERIAL_H := $(RUNTIME_ROOT)/supervisor/shared/serial.h
RUNTIME_CONFIG_H := $(RUNTIME_ROOT)/supervisor/shared/r2p2_runtime_config.h
RUNTIME_BOARD_MK := $(RUNTIME_ROOT)/ports/nordic/boards/$(BOARD)/mpconfigboard.mk

.PHONY: prepare-runtime-tree build-cdc-dual clean

build-cdc-dual: prepare-runtime-tree
	$(MAKE) -C $(RUNTIME_ROOT)/ports/nordic \
		-f $(RUNTIME_NORDIC_MAKEFILE) \
		BOARD=$(BOARD) \
		BUILD=$(BUILD_DIR) \
		CP_VERSION=$(CP_VERSION)

clean:
	rm -rf $(BUILD_ROOT)

prepare-runtime-tree:
	rm -rf $(RUNTIME_ROOT)
	mkdir -p $(BUILD_ROOT)
	cp -R $(VENDOR_ROOT) $(RUNTIME_ROOT)
	cp $(ROOT_RUNTIME_NORDIC_MAKEFILE) $(RUNTIME_NORDIC_MAKEFILE)
	cp $(ROOT_RUNTIME_SUPERVISOR_MK) $(RUNTIME_SUPERVISOR_MK)
	cp $(ROOT_RUNTIME_USB_CDC_INIT) $(RUNTIME_USB_CDC_INIT)
	cp $(ROOT_RUNTIME_USB_CDC_MODULE_H) $(RUNTIME_USB_CDC_MODULE_H)
	cp $(ROOT_RUNTIME_USB_C) $(RUNTIME_USB_C)
	cp $(ROOT_RUNTIME_USB_DESC_C) $(RUNTIME_USB_DESC_C)
	cp $(ROOT_RUNTIME_USB_DEVICE_C) $(RUNTIME_USB_DEVICE_C)
	cp $(ROOT_RUNTIME_TUSB_CONFIG_H) $(RUNTIME_TUSB_CONFIG_H)
	cp $(ROOT_RUNTIME_SERIAL_C) $(RUNTIME_SERIAL_C)
	cp $(ROOT_RUNTIME_SERIAL_H) $(RUNTIME_SERIAL_H)
	cp $(ROOT_RUNTIME_CONFIG_H) $(RUNTIME_CONFIG_H)
	cp $(BOARD_OVERRIDE_MK) $(RUNTIME_BOARD_MK)
