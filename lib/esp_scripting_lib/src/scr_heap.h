/* scr_heap.h
** Виртуальная память
*/

#pragma once
#ifndef SCR_HEAP_H
#define SCR_HEAP_H

#include <cstring>
#include "scr_types.h"
#include "config/scr_config.h"
#include "utils/scr_debug.h"
#include "scr_vm_errors.h"

#define HEAP_MIN_SIZE 256
#define HEAP_MAX_SIZE 65535

#define HEAP_ID_FREE 0

// Макросы для битовых операций
#define BITREAD(NUM, BIT) (((NUM) >> (BIT)) & 1)
#define BITSET(NUM, BIT) ((NUM) |= (1UL << (BIT)))
#define BITCLEAR(NUM, BIT) ((NUM) &= ~(1UL << (BIT)))
#define BITWRITE(NUM, BIT, VAL) ((VAL) ? BITSET((NUM), (BIT)) : BITCLEAR((NUM), (BIT)))

// Маска для типа блока - 2 старших бита
#define HEAP_TYPE_MASK 0xC0000000U
/* Т.к. ид блока занимает только 14 бит, нужно убирать первые 2*/
#define HEAP_ID_MASK 0x3FFFU

typedef uint8_t  heap_t;
typedef uint16_t u16_t;

#if defined(VM_USE_FIXED_MEMORY) && VM_DEFAULT_HEAP_SIZE > 65535
	typedef uint32_t heap_size_t;
#else
	typedef uint16_t heap_size_t;
#endif

// Заголовок блока
typedef struct {
	u16_t id;
	heap_size_t len;
} heaphdr_t;

class Heap {
	public:
		Heap(uint32_t size);
		~Heap();
		void heap_dmp(void);
		u16_t heapNewId(void);
		bool heapAllocInternal(u16_t id, heap_size_t size);
		u16_t alloc(heap_size_t size);
		void realloc(u16_t id, heap_size_t size);
		heap_size_t getLength(u16_t id);
		void* getAddr(u16_t id);
		heaphdr_t* getHeader(u16_t id);
		heap_size_t getFreeSize(void);
		uint32_t getTotalSize(void);
		heap_t* getRelativeBase(void);
		heap_t* getBase(void);
		void garbageCollect(void);
		void memSteal(uint32_t length);
		void memUnsteal(uint32_t length);
		// Поля
#ifndef VM_USE_FIXED_MEMORY
		heap_t *_heap; // Указатель на начало
#endif
		uint32_t _base; // Начало пустого блока
		uint32_t _heap_size; // Размер
		uint16_t _alloc_id; // Самое большое доступное значение ид
		stack_t *_sb; // Начало стека
		stack_t *_fp;
		stack_t *_locals; // Переменные
};

#endif // SCR_HEAP_H