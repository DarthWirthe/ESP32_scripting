
#pragma once
#ifndef NATIVE_ARRAY_H
#define NATIVE_ARRAY_H

#include "scr_lib.h"

static int A_array_length(stack_t *&sp) {
    uint32_t id = POP();
    uint32_t len = heap_ref()->getLength(id) / 4;
    PUSH(len);
    return 1;
}

const struct lib_reg native_array[] = {
    {"length", A_array_length, 2, new VarType[2] {
        VarType::Std::Int, VarType(VarType::Std::Int, true),
    }}
};

#endif