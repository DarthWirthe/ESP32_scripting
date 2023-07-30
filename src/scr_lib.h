
#pragma once
#ifndef SCR_LIB_H
#define SCR_LIB_H

#include <stddef.h>
#include <vector>
#include "scr_types.h"
#include "scr_builtin_types.h"
#include "scr_heap.h"
#include "scr_tref.h"
#include "utils/scr_debug.h"
#include "libs/hal/scr_hal.h"

#define NO_LABEL nullptr

// Макросы для работы со стеком

#define PUSH(v) *++sp = (v)
#define POP() 	(*sp--)
#define TOP()   (*sp)

/* Указатель на встроенную функцию 
 * stack_t *&sp - указатель стека
*/
typedef int (*cfunction)(stack_t*&);

/* name - название функции
 * str_ref - отсылка на название
 * func - функция
*/
struct lib_reg {
    const char *name;
    cfunction func;
    int args_count;
    VarType *args;
};

/* name - название библиотеки
 * str_ref - отсылка на название
 * lib_reg - функции
 * size - кол-во функций
*/
struct lib_struct {
    const char *name;
    size_t str_ref;
    lib_reg *lib;
    size_t size;
};

// Структуры для code.h
struct lib_reg_c {
    const char *name;
    int str_ref;
    cfunction func;
    int id;
    int args_count;
    VarType *args;
};

struct lib_struct_c {
    const char *name;
    lib_reg_c *lib;
    size_t size;
};

// ### //

void VMInternalException(void);
void set_heap_ref(Heap *ref);
Heap* heap_ref();
int_t stack_to_int(stack_t val);
uint32_t VM_LoadString(Heap *heap, char *c);
void setSDEnabledFlag(bool v);
void setSPIFFSEnabledFlag(bool v);
bool isSDEnabled(void);
bool isSPIFFSEnabled(void);
cfunction* GET_CFPT(const char *src, int *function_reg, size_t len, std::vector<lib_struct> libs);
cfunction* GET_CFPT(const char *src, size_t count, std::vector<cfunction> f_pts);

#endif // SCR_LIB_H