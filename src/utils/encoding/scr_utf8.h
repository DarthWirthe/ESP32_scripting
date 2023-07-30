
#pragma once
#ifndef SCR_UTF8_H
#define SCR_UTF8_H

#define IS_UTF8_CL(c) ((c) == 0xD0 || (c) == 0xD1)
// Возвращает TRUE если символ utf-8 из кириллицы
static inline bool IS_UTF8_CYR(char cl, char c) {
    return (cl == 0xD0 && ((c >= 0x90 && c <= 0xBF) || c == 0x81)) || 
    (cl == 0xD1 && ((c >= 0x80 && c <= 0x8F) || c == 0x91));
}

#endif