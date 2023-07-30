
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

static int VM_get_heap_size(stack_t *&sp) {
    uint32_t size = heap_ref()->getHeapSize();
    PUSH(size);
    return 1;
}

static int VM_get_heap_blocks_count(stack_t *&sp) {
    uint32_t free_mem = heap_ref()->getBlocksCount();
    PUSH(free_mem);
    return 1;
}

static int VM_collect_garbage(stack_t *&sp) {
    uint32_t freed_mem = heap_ref()->garbageCollect();
    PUSH(freed_mem);
    return 1;
}

const struct lib_reg lib_native_vm[] = {
    /* int vm.get_total_memory_size() */
    {"get_total_memory_size", VM_total_memory, 1, new VarType[1] {
        VarType::Std::Int,
    }},
    /* int vm.get_free_memory_size() */
    {"get_free_memory_size", VM_free_memory, 1, new VarType[1] {
        VarType::Std::Int,
    }},
    /* int vm.get_heap_size() */
    {"get_heap_size", VM_get_heap_size, 1, new VarType[1] {
        VarType::Std::Int,
    }},
    /* int vm.get_free_memory_size() */
    {"get_heap_blocks_count", VM_get_heap_blocks_count, 1, new VarType[1] {
        VarType::Std::Int,
    }},
    /* int vm.collect_garbage() */
    {"collect_garbage", VM_collect_garbage, 1, new VarType[1] {
        VarType::Std::Int,
    }}
};

#endif