
#pragma once
#ifndef SCR_TYPES_H
#define SCR_TYPES_H

#include <stdint.h> // для фиксированных типов

typedef uint32_t stack_t;

#define TYPE_HEAP_PT_MASK 0xDEAD0000L

#define IMMEDIATE_MASK_16 0x8000 // для знаковых типов 16 бит
#define IMMEDIATE_MASK_32 0x80000000 // для знаковых типов 32 бит

typedef union {
	int   i;
	float f;
} num_t;

typedef int8_t  char_t;
typedef int16_t short_t;
typedef int32_t int_t;

typedef union {
    char  b[4];
    short s[2];
    int   i[1];
    float f[1];
} union_t;

int_t stack_to_int(uint32_t val);
short_t uint_to_short16(uint16_t val);
stack_t float_to_stack(float val);
float stack_to_float(stack_t val);

#endif