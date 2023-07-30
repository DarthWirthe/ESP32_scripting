
/** scr_vm.h
*** Заголовочный файл.
*** Виртуальная машина.
**/

#pragma once
#ifndef SCR_VM_H
#define SCR_VM_H

#include "scr_vm_opcodes.h"
#include "scr_vm_errors.h"
#include "scr_types.h"
#include "scr_heap.h"
#include "config/scr_config.h"
#include "utils/scr_debug.h"
#include "scr_lib.h"

typedef union {
    uint16_t u16;
    int16_t i16;
    struct {
        uint8_t l, h;
    } u8;
    int_t i32;
} arg_t;

// Заголовок кадра (frame header)
typedef struct {
    stack_t *fp;
    stack_t *return_pt;
    stack_t *locals_pt;
    uint8_t *func_ref;
} frame_hdr_t;

// Размер заголовка кадра (слов)
constexpr unsigned int FRAME_REQUIREMENTS = 4U;

class VM
{
	public:
		VM(void);
		VM(uint32_t h_size, uint8_t *ptcode, num_t *nums,
                char *strs, unsigned int g_count, cfunction *funcs);
		~VM();
        int run(void);
	private:
		Heap *heap_;
		uint8_t *codeBase_; // указатель на начало кода
		num_t *numConstsBase_;
        char *strConstsBase_;
        cfunction *functionReference_;
        unsigned int globalsCount_;
};

#endif // SCR_VM_H