
#pragma once
#ifndef NATIVE_VM_H
#define NATIVE_VM_H

#include "scr_lib.h"

static int VM_total_memory(stack_t *&sp) {
    uint32_t free_mem = heap_ref()->getTotalSize();
    PUSH(free_mem);
    return 1;
}

static int VM_free_memory(stack_t *&sp) {
    uint32_t free_mem = heap_ref()->getFreeSize();
    PUSH(free_mem);
    return 1;
}

const struct lib_reg lib_native_vm[] = {
    /* int vm.get_total_memory_size() */
    {"get_total_memory_size", VM_total_memory, 1, new VarType[1] {
        VarType::Int,
    }},
    /* int vm.get_free_memory_size() */
    {"get_free_memory_size", VM_free_memory, 1, new VarType[1] {
        VarType::Int,
    }}
};

#endif