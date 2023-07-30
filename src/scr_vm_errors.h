/*
**	Заголовочный файл.
**	Коды ошибок виртуальной машины.
*/

#pragma once
#ifndef SCR_VM_ERRORS_H
#define SCR_VM_ERRORS_H

#define VM_NOERROR 0x00
#define VM_ERR_ILLEGAL_INSTRUCTION 0x01
#define VM_ERR_EXPECTED_FUNCTION 0x08
#define VM_ERR_INDEX_OUT_OF_RANGE 0x09
#define VM_ERR_MISSING_DATA 0x0a
#define VM_FUNCTION_EXCEPTION 0x0b

#define HEAP_ERR_NOT_FOUND 0x20
#define HEAP_ERR_OUT_OF_MEMORY 0x21
#define HEAP_ERR_OUT_OF_RAM 0x22
#define HEAP_ERR_CORRUPTED 0x23
#define HEAP_ERR_OVERFLOW 0x24
#define HEAP_ERR_UNDERFLOW 0x25
#define HEAP_ERR_OUT_OF_IDS 0x26

#endif