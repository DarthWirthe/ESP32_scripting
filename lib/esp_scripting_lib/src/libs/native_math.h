
#pragma once
#ifndef NATIVE_MATH_H
#define NATIVE_MATH_H

#include "scr_lib.h"

#include <cmath>

static int M_sqrt(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(sqrtf(v1)));
    return 1;
}

static int M_pow(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    float v2 = stack_to_float(POP());
    PUSH(float_to_stack(powf(v2, v1)));
    return 1;
}

static int M_sin(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(sinf(v1)));
    return 1;
}

static int M_cos(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(cosf(v1)));
    esp_random();
    return 1;
}

static int M_tan(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(tanf(v1)));
    return 1;
}

static int M_random(stack_t *&sp) {
    int big_l = stack_to_int(POP());
    int small_l = stack_to_int(POP());
    PUSH(esp_random2(small_l, big_l));
    return 1;
}

const struct lib_reg native_math[] = {
    {"sqrt", M_sqrt, 2, new VarType[2] {
        VarType::Float, VarType::Float
    }},
    {"pow", M_pow, 3, new VarType[3] {
        VarType::Float, VarType::Float, VarType::Float
    }},
    {"sin", M_sin, 2, new VarType[2] {
        VarType::Float, VarType::Float
    }},
    {"cos", M_cos, 2, new VarType[2] {
        VarType::Float, VarType::Float
    }},
    {"tan", M_tan, 2, new VarType[2] {
        VarType::Float, VarType::Float
    }},
    {"random", M_random, 3, new VarType[3] {
        VarType::Int, VarType::Int, VarType::Int
    }}
};

#endif