
#pragma once
#ifndef LIB_GPIO_H
#define LIB_GPIO_H

#include "scr_lib.h"

static bool pwm_enabled = false;



static int GPIO_set_mode_input(stack_t *&sp) {
    uint32_t pin = POP();
    pinMode(pin, INPUT);
    return 1;
}

static int GPIO_set_mode_output(stack_t *&sp) {
    uint32_t pin = POP();
    pinMode(pin, OUTPUT);
    return 1;
}

static int GPIO_write(stack_t *&sp) {
    uint32_t value = POP();
    uint32_t pin = POP();
    digitalWrite(pin, value);
    return 1;
}

static int GPIO_read(stack_t *&sp) {
    uint32_t pin = POP();
    PUSH(digitalRead(pin));
    return 1;
}

static int GPIO_adc_read(stack_t *&sp) {
    uint32_t pin = POP();
    PUSH(analogRead(pin));
    return 1;
}

static int GPIO_adc_resolution(stack_t *&sp) {
    uint32_t res = POP();
    // Значение 9 - 12
    analogReadResolution(res);
    return 1;
}

static int GPIO_pwm_setup(stack_t *&sp) {
    uint32_t resolution = POP();
    uint32_t channel = POP();
    resolution = (resolution < 1) ? 1 : ((resolution > 16) ? 16 : resolution);
    ledcSetup(channel, 20000, resolution);
    pwm_enabled = true;
    return 1;
}

static int GPIO_pwm_attach_pin(stack_t *&sp) {
    uint32_t channel = POP();
    uint32_t pin = POP();
    ledcAttachPin(pin, channel);
    return 1;
}

static int GPIO_pwm_detach_pin(stack_t *&sp) {
    uint32_t pin = POP();
    ledcDetachPin(pin);
    return 1;
}

static int GPIO_pwm_write(stack_t *&sp) {
    uint32_t value = POP();
    uint32_t chan = POP();
    if (pwm_enabled)
        ledcWrite(chan, value);
    return 1;
}

const struct lib_reg lib_gpio[] = {
    /* set_mode_input(int pin) */
    {"set_mode_input", GPIO_set_mode_input, 2, new VarType[2] {
        VarType::None, VarType::Int,
    }},
    /* set_mode_output(int pin) */
    {"set_mode_output", GPIO_set_mode_output, 2, new VarType[2] {
        VarType::None, VarType::Int,
    }},
    /* write(int pin, int value) */
    {"write", GPIO_write, 3, new VarType[3] {
        VarType::None, VarType::Int, VarType::Int
    }},
    /* int read(int pin) */
    {"read", GPIO_read, 2, new VarType[2] {
        VarType::Int, VarType::Int,
    }},
    /* int adc_read(int pin) */
    {"adc_read", GPIO_adc_read, 2, new VarType[2] {
        VarType::Int, VarType::Int,
    }},
    /* set_adc_resolution(int resolution) */
    {"set_adc_resolution", GPIO_adc_resolution, 2, new VarType[2] {
        VarType::None, VarType::Int,
    }},
    /* pwm_setup(int adc_channel, int resolution) */
    {"pwm_setup", GPIO_pwm_setup, 3, new VarType[3] {
        VarType::None, VarType::Int, VarType::Int
    }},
    /* pwm_attach_pin(int pin, int adc_channel) */
    {"pwm_attach_pin", GPIO_pwm_attach_pin, 3, new VarType[3] {
        VarType::None, VarType::Int, VarType::Int
    }},
    {"pwm_detach_pin", GPIO_pwm_detach_pin, 2, new VarType[2] {
        VarType::None, VarType::Int
    }},
    /* pwm_write(int channel, int value) */
    {"pwm_write", GPIO_pwm_write, 3, new VarType[3] {
        VarType::None, VarType::Int, VarType::Int
    }}
};

#endif