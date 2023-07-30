
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

static int M_log(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(logf(v1)));
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
    return 1;
}

static int M_tan(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(tanf(v1)));
    return 1;
}

static int M_asin(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(asinf(v1)));
    return 1;
}

static int M_acos(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(acosf(v1)));
    return 1;
}

static int M_atan(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(atanf(v1)));
    return 1;
}

static int M_abs(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(abs(v1)));
    return 1;
}

static int M_exp(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(expf(v1)));
    return 1;
}

static int M_floor(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(floorf(v1)));
    return 1;
}

static int M_ceil(stack_t *&sp) {
    float v1 = stack_to_float(POP());
    PUSH(float_to_stack(ceilf(v1)));
    return 1;
}

static int M_random(stack_t *&sp) {
    int big_l = stack_to_int(POP());
    int small_l = stack_to_int(POP());
    PUSH(esp_random2(small_l, big_l));
    return 1;
}

const struct lib_reg native_math[] = {
    /* float m.sqrt(float n) */
    {"sqrt", M_sqrt, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.pow(float n1, float n2) */
    {"pow", M_pow, 3, new VarType[3] {
        VarType::Std::Float, VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.log(float n) */
    {"log", M_log, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.sin(float n) */
    {"sin", M_sin, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.cos(float n) */
    {"cos", M_cos, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.tan(float n) */
    {"tan", M_tan, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.asin(float n) */
    {"asin", M_asin, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.acos(float n) */
    {"acos", M_acos, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.atan(float n) */
    {"atan", M_atan, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.abs(float n) */
    {"abs", M_abs, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.exp(float n) */
    {"exp", M_exp, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.floor(float n) */
    {"floor", M_floor, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* float m.ceil(float n) */
    {"ceil", M_ceil, 2, new VarType[2] {
        VarType::Std::Float, VarType::Std::Float
    }},
    /* int m.random(int small, int big) */
    {"random", M_random, 3, new VarType[3] {
        VarType::Std::Int, VarType::Std::Int, VarType::Std::Int
    }}
};

#endif