// This file is part of the R2P2-nRF52 runtime.
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void serial_early_init(void);
void serial_init(void);
void serial_write(const char *text);
// Only writes up to given length. Does not check for null termination at all.
uint32_t serial_write_substring(const char *text, uint32_t length);
char serial_read(void);
uint32_t serial_bytes_available(void);
bool serial_connected(void);

// Used for temporarily suppressing output to the console.
bool serial_console_write_disable(bool disabled);

int console_uart_printf(const char *fmt, ...);
