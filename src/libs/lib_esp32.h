
#pragma once
#ifndef LIB_ESP32_H
#define LIB_ESP32_H

#include "scr_lib.h"

#include "Esp.h"

static int ESP32_timer(stack_t *&sp) {
    PUSH((stack_t)esp_uptime());
    return 1;
}

static int ESP32_timer_ms(stack_t *&sp) {
    PUSH((stack_t)(esp_uptime() / 1000));
    return 1;
}

static int ESP32_free_memory(stack_t *&sp) {
    PUSH(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    return 1;
}

static int ESP32_get_model(stack_t *&sp) {
    const char *b = ESP.getChipModel();
    stack_t id = VM_LoadString(heap_ref(), (char*)b);
    PUSH(id);
    return 1;
}

const struct lib_reg lib_esp32[] = {
    {"timer", ESP32_timer, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    {"timer_ms", ESP32_timer_ms, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    {"free_memory", ESP32_free_memory, 1, new VarType::Std[1] {
        VarType::Std::Int
    }},
    {"get_model", ESP32_get_model, 1, new VarType::Std[1] {
        VarType::Std::String
    }}
};

#endif