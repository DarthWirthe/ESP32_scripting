
#pragma once
#ifndef NATIVE_CONVERT_H
#define NATIVE_CONVERT_H

#include "scr_lib.h"

#include "stdlib_noniso.h"

static int C_int_to_string(stack_t *&sp) {
    int num = stack_to_int(POP());
    char buffer[17];
    itoa(num, buffer, 10);
    stack_t id = VM_LoadString(heap_ref(), buffer);
    PUSH(id);
    return 1;
}

static int C_float_to_string(stack_t *&sp) {
    float num = stack_to_float(POP());
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%f", num);
    stack_t id = VM_LoadString(heap_ref(), buffer);
    PUSH(id);
    return 1;
}

const struct lib_reg native_convert[] = {
    /* string int_to_string(int n) */
    {"int_to_string", C_int_to_string, 2, new VarType[2] {
        VarType::Std::String, VarType::Std::Int
    }},
    /* string float_to_string(float n) */
    {"float_to_string", C_float_to_string, 2, new VarType[2] {
        VarType::Std::String, VarType::Std::Float
    }}
};

#endif