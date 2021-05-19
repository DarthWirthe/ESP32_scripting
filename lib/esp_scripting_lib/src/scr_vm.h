
/** scr_vm.h
*** Заголовочный файл.
*** Виртуальная машина.
**/

#pragma once
#ifndef SCR_VM_H
#define SCR_VM_H

#include <string>
#include "scr_types.h"
#include "scr_heap.h"
#include "scr_vm_opcodes.h"
#include "scr_vm_errors.h"
#include "config/scr_config.h"
#include "scr_lib.h"

#ifdef ESP32
    #include "soc/rtc_wdt.h"
#endif

#define FRAME_REQUIREMENTS 4

typedef union {
    uint16_t U16;
    int16_t I16;
    struct {
        uint8_t L, H;
    } U8;
    int_t I32;
} arg_t;

// Заголовок кадра (frame header)
typedef struct {
    stack_t *fp;
    stack_t *return_pt;
    stack_t *locals_pt;
    uint8_t *func_ref;
} frame_hdr_t;

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