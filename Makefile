BOARD ?= ssci_isp1807_dev_board

ROOT := $(abspath .)
BUILD_DIR := $(ROOT)/build/$(BOARD)
SRC_DIR := $(ROOT)/src
INCLUDE_DIR := $(ROOT)/include
BUILD_CONFIG_DIR := $(ROOT)/build_config
COMPONENTS_DIR := $(ROOT)/components
PICORUBY_NRF52_DIR := $(COMPONENTS_DIR)/picoruby-nRF52

SRC_C := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/serial_transport.c \
	$(SRC_DIR)/usb_cdc_transport.c \
	$(SRC_DIR)/usb_descriptors.c \
	$(SRC_DIR)/usb_device.c \
	$(SRC_DIR)/usb_runtime.c

INC_DIRS := \
	$(INCLUDE_DIR) \
	$(PICORUBY_NRF52_DIR)

.PHONY: build-cdc-dual clean

build-cdc-dual:
	@echo "R2P2-nRF52 product build graph rewrite in progress."
	@echo "Board: $(BOARD)"
	@echo "Sources:"
	@printf '  %s\n' $(SRC_C)
	@echo "Includes:"
	@printf '  %s\n' $(INC_DIRS)
	@echo "Next step: wire startup, linker, nrfx, TinyUSB, and UF2 generation here."
	@false

clean:
	rm -rf $(BUILD_DIR)
