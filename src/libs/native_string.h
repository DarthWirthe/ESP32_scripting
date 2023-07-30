
#pragma once
#ifndef NATIVE_STRING_H
#define NATIVE_STRING_H

#include "scr_lib.h"
#include "stdlib_noniso.h"

static int S_string_length(stack_t *&sp) {
    uint32_t id = POP();
    uint32_t len = heap_ref()->getLength(id) - 1;
    PUSH(len);
    return 1;
}

static int S_string_compare(stack_t *&sp) {
    int res;
    stack_t id1 = POP();
    stack_t id2 = POP();
    char *s1 = (char*)heap_ref()->getAddr(id1);
    char *s2 = (char*)heap_ref()->getAddr(id2);
    res = strcmp(s1, s2);
    if (res == 0)
        PUSH(1);
    else
        PUSH(0);
    return 1;
}

static int S_string_substr(stack_t *&sp) {
    Heap *heap = heap_ref();
    int len = stack_to_int(POP()); // to
    int st = stack_to_int(POP()); // from
    stack_t id1 = POP(); // string
    heaphdr_t *hdr = heap->getHeader(id1);
    heap_size_t strl = hdr->len;
    st = (st < 0 || st > strl - 1) ? 0 : st;
    len = (st + len > strl - 1) ? strl - st : len;
    uint8_t *pos = (uint8_t*)(hdr + 1) + st;
    uint32_t id2 = heap->alloc(len + 1);
    void *addr = heap->getAddr(id2);
    memcpy(addr, pos, len);
    *((char*)addr + len) = 0;
    PUSH(id2);
    return 1;
}

static int S_string_concat(stack_t *&sp) {
    Heap *heap = heap_ref();
    stack_t id2 = POP(); // string 2
    stack_t id1 = POP(); // string 1
    heaphdr_t *hdr1 = heap->getHeader(id1);
    heaphdr_t *hdr2 = heap->getHeader(id2);
    heap_size_t strl1 = hdr1->len - 1;
    heap_size_t strl2 = hdr2->len - 1;
    stack_t id3 = heap->alloc(strl1 + strl2 + 1);
    uint8_t *pos1 = (uint8_t*)heap->getAddr(id3);
    uint8_t *pos2 = pos1 + strl1;
    memcpy(pos1, hdr1 + 1, strl1);
    memcpy(pos2, hdr2 + 1, strl2);
    *((char*)pos2 + strl2) = 0;
    PUSH(id3);
    return 1;
}

const struct lib_reg native_string[] = {
    /* int string.length(string s) */
    {"length", S_string_length, 2, new VarType[2] {
        VarType::Std::Int, VarType::Std::String,
    }},
    /* int string.length(string s1, string s2) */
    {"compare", S_string_compare, 3, new VarType[3] {
        VarType::Std::Int, VarType::Std::String, VarType::Std::String
    }},
    /* string string.length(string s, int start, int end) */
    {"substr", S_string_substr, 4, new VarType[4] {
        VarType::Std::String, VarType::Std::String, VarType::Std::Int, VarType::Std::Int
    }},
    {"concat", S_string_concat, 3, new VarType[3] {
        VarType::Std::String, VarType::Std::String, VarType::Std::String
    }}
};

#endif