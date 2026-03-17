#ifndef SERIAL_TRANSPORT_H
#define SERIAL_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

void serial_early_init(void);
void serial_init(void);
void serial_write(const char *text);
uint32_t serial_write_substring(const char *text, uint32_t length);
char serial_read(void);
uint32_t serial_bytes_available(void);
bool serial_connected(void);
bool serial_console_write_disable(bool disabled);
int console_uart_printf(const char *fmt, ...);

#endif
