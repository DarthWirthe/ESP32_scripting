
#pragma once
#ifndef LIB_UART_H
#define LIB_UART_H

#include "scr_lib.h"

static int UART_print(stack_t *&sp) {
    uint32_t v1 = POP();
    char *str = (char*)(heap_ref()->getAddr(v1));
    PRINTF(str);
    return 1;
}

const struct lib_reg lib_uart[] = {
    /* uart.print(string s) */
    {"write", UART_print, 2, new VarType[2] {
        VarType::Std::None, VarType::Std::String,
    }}
};

#endif