
#pragma once
#ifndef INCLUDE_LIBS_H
#define INCLUDE_LIBS_H

/**/
#include "libs/lib_uart.h"
#include "libs/native_math.h"
#include "libs/native_string.h"
#include "libs/native_array.h"
#include "libs/native_vm.h"
#include "libs/native_convert.h"

#ifdef CONFIG_ENABLE_GPIO_LIB
    #include "libs/lib_gpio.h"
#endif

#ifdef CONFIG_ENABLE_ESP32_LIB
    #include "libs/lib_esp32.h"
#endif

#ifdef CONFIG_ENABLE_TFT_LIB
    #include "libs/lib_tft.h"
#endif

#ifdef CONFIG_ENABLE_ESP_VGA_LIB
    #include "libs/lib_esp_vga.h"
#endif

#endif
