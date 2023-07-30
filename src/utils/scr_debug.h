
#pragma once
#ifndef SCR_DEBUG_H
#define SCR_DEBUG_H

#include <cstddef>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <HardwareSerial.h>

#define __DEBUGF__(__f, ...) PRINTF(__f, ##__VA_ARGS__)

void WRITE(uint8_t *buffer);
size_t PRINTF(const char *format, ...);
size_t PRINT_ERROR(const char *format, ...);
std::string DEBUG_GET_ERROR_MESSAGE(void);
void debug_clear_error_message(void);

#endif