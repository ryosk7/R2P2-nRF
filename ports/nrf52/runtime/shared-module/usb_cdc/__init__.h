// This file is part of the R2P2-nRF52 runtime.
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "supervisor/shared/r2p2_runtime_config.h"
#include "supervisor/usb.h"

bool usb_cdc_console_enabled(void);
bool usb_cdc_data_enabled(void);
uint8_t usb_cdc_console_index(void);
uint8_t usb_cdc_data_index(void);

void usb_cdc_set_defaults(void);
bool usb_cdc_enable(bool console, bool data);
bool usb_cdc_disable(void);

size_t usb_cdc_descriptor_length(void);
size_t usb_cdc_add_descriptor(uint8_t *descriptor_buf, descriptor_counts_t *descriptor_counts, uint8_t *current_interface_string, bool console);
