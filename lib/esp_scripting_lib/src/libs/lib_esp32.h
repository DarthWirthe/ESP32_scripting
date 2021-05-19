
#pragma once
#ifndef LIB_ESP32_H
#define LIB_ESP32_H

#include "scr_lib.h"

static int ESP32_timer(stack_t *&sp) {
    PUSH((stack_t)esp_uptime());
    return 1;
}

const struct lib_reg lib_esp32[] = {
    {"timer", ESP32_timer, 1, new VarType[1] {
        VarType::Int
    }}
};

#endif