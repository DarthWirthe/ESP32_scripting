/* scr_heap.cpp
** Виртуальная память
*/

#include "scr_heap.h"

#ifdef VM_USE_FIXED_MEMORY
#if VM_DEFAULT_HEAP_SIZE > 102400
#error "VM_DEFAULT_HEAP_SIZE must be <= 102400"
#endif
heap_t heap[VM_DEFAULT_HEAP_SIZE];
#endif

// Макрос для ошибок
#define HEAP_ERR(err) throw (err)

// Макрос для отладки
#ifdef H_DBG
	#define DEBUGF(__f, ...) __DEBUGF__(__f, ##__VA_ARGS__)
#else
	#define DEBUGF(__f, ...)
#endif

static inline void heap_memcpy_up(uint8_t *dst, uint8_t *src, uint16_t len) {
	dst += len;
	src += len;
	while(len--) *--dst = *--src;
}

// Инициализация
Heap::Heap(uint32_t size) {
	heaphdr_t *h;
#ifndef VM_USE_FIXED_MEMORY
	_heap = (heap_t*)malloc(sizeof(heap_t) * size);
	if (!_heap)
		HEAP_ERR(HEAP_ERR_OUT_OF_RAM); // Недостаточно памяти ОЗУ
#endif
	_base = 0;
	_alloc_id = 1;
	// Один пустой блок занимает всё пространство
	h = (heaphdr_t*)&_heap[0];
	h->id = HEAP_ID_FREE;
#ifndef VM_USE_FIXED_MEMORY
	_heap_size = size;
	h->len = size - sizeof(heaphdr_t);
#else
	this->length = VM_DEFAULT_HEAP_SIZE;
	h->len = VM_DEFAULT_HEAP_SIZE - sizeof(heaphdr_t);
#endif
}

Heap::~Heap() {
#ifndef VM_USE_FIXED_MEMORY
	free(_heap);
#endif
}

void Heap::heap_dmp() {
	uint16_t current = _base;
	PRINTF("HEAP: DUMP()\n");
	while (current < _heap_size) {
		heaphdr_t *h = (heaphdr_t*)(_heap + current);
		uint8_t *begin = (uint8_t*)(h + 1);
		uint16_t id = h->id & HEAP_ID_MASK;
		uint16_t len = h->len + sizeof(heaphdr_t);
		current += len;
		PRINTF("Chunk id=0x%04x size=%u addr=0x%08x\ndata:\n", id, h->len, h);
		for (uint32_t i = 0; i < h->len; i++){
			PRINTF("0x%08x ", *((uint32_t*)begin + i));
			if (!(i%8))
				PRINTF("\n");
		}
		PRINTF("\n");
	}
}

// Поиск блока с идентификатором
static inline heaphdr_t* heap_search(heap_t *heap, uint16_t base, uint16_t length, u16_t id) {
	uint32_t current = base;
	while (current < length) {
		heaphdr_t *h = (heaphdr_t*)(heap + current);
		if((h->id & HEAP_ID_MASK) == id) return h;
		current += h->len + sizeof(heaphdr_t);
	}
	return 0;
}

// Поиск свободного блока
u16_t Heap::heapNewId() {
	u16_t id;
	if (!_alloc_id) {
		for (id = 1; id; id++) 
			if (!heap_search(_heap, _base, _heap_size, id))
				return id;
	} else if (_alloc_id == HEAP_ID_MASK) { // номерки закончились
		return 0;
	} else {
		return ++_alloc_id;
	}
	return 0;
}

// Выделение памяти по идентификатору
bool Heap::heapAllocInternal(u16_t id, heap_size_t size) {
	uint16_t required = size + sizeof(heaphdr_t);
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	if (h->len >= required) { // если есть свободное пространство
		h->len -= required; // уменьшение свободного блока
		h = (heaphdr_t*)(_heap + _base + sizeof(heaphdr_t) + h->len);
		h->id = id;
		h->len = size;
#ifdef HEAP_INIT_ALLOCATED
		// заполнение нулями
		uint8_t *ptr = (uint8_t*)(h + 1);
		memset(ptr, 0x00, sizeof(heap_t) * size);
#endif
		return true;
	}
	return false;
}

// Создание блока
u16_t Heap::alloc(heap_size_t size) {
	u16_t id = heapNewId();
	
	if (!id) {
		garbageCollect(); // сборка мусора
		_alloc_id = 0;
		id = heapNewId();
		if (!id)
			HEAP_ERR(HEAP_ERR_OUT_OF_IDS); /* Закончились номерки */
		garbageCollect(); // сборка мусора
	}
	if (!heapAllocInternal(id, size)) {
		// Если нет свободного пространства:
		garbageCollect(); // сборка мусора
		//id = heap_new_id(); // ид был переопределён
		if (!id) {
			_alloc_id = 0;
			id = heapNewId();
			if (!id)
				HEAP_ERR(HEAP_ERR_OUT_OF_IDS); /* Закончились номерки */
		}
		if (!heapAllocInternal(id, size)) // попробовать снова
			HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	}
	DEBUGF("Heap::alloc(): id=%u, size=%u\n", id, size);
	return id;
}

// Изменение размера блока
void Heap::realloc(u16_t id, heap_size_t size) {
	heaphdr_t *h, *h_new;
	// заголовок старого блока
	h = (heaphdr_t*)(_heap + _base);
	if(h->len >= size + sizeof(heaphdr_t))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	h = heap_search(_heap, _base, _heap_size, id);
	// создание нового блока
	if (!heapAllocInternal(id, size))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	h_new = heap_search(_heap, _base, _heap_size, id);
	memcpy(h_new + 1, h + 1, h->len);
	// Старый блок будет удалён сборщиком
	h->id = HEAP_ID_FREE;
}

// Возвращает длину блока
heap_size_t Heap::getLength(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _base, _heap_size, id);
	if (!h) {
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	}
	return h->len;
}

// Возвращает адрес данных в блоке
void* Heap::getAddr(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _base, _heap_size, id);
	if (!h)
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	return h + 1;
}

// Возвращает адрес заголовка блока
heaphdr_t* Heap::getHeader(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _base, _heap_size, id);
	if (!h)
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	return h;
}

// Возвращает размер виртуальной памяти
uint32_t Heap::getTotalSize() {
	return _heap_size;
}

// Возвращает размер свободного блока
heap_size_t Heap::getFreeSize() {
	return ((heaphdr_t*)(_heap + _base))->len;
}

// Возвращает указатель на начало свободного блока
heap_t* Heap::getRelativeBase() {
	return _heap + _base;
}

heap_t* Heap::getBase() {
#ifdef VM_USE_FIXED_MEMORY
	return heap;
#else
	return _heap;
#endif
}

bool heapIdInUse(uint16_t id, stack_t *_base, stack_t *fp) {
	if (!fp)
		return false;
	stack_t *locals = (stack_t*)fp[2];
	stack_t ref_id = id | TYPE_HEAP_PT_MASK;
	//PRINTF("heap_id_in_use id %u (%u)\n", id, ref_id);
	stack_t *pt = nullptr;
	while (fp) {
		//PRINTF("fp=%u, locals=%u\n", fp, locals);
		pt = fp - 1;
		// поиск по кадру
		for (;pt != locals - 1; pt--) {
			//PRINTF("%u\n", *pt);
			if (*pt == ref_id)
				return true;
		}
		// посмотрим в пред. кадре
		fp = (stack_t*)fp[0];
		if (fp != nullptr)
			locals = (stack_t*)fp[2];
	}
	pt += 1;
	// посмотрим в глобальных переменных
	//PRINTF("##PT=%u,BASE=%u\n",pt,_base);
	for (;pt != _base; pt--) {
		if (*pt == ref_id)
			return true;
	}
	return false;
}

/* Функция сборки мусора
 * Проверяет каждый блок в куче на достижимость
 * если объект не используется, то он удаляется из памяти
*/
void Heap::garbageCollect() {
	DEBUGF("HEAP: garbageCollect()\nBefore: %u bytes free\n", getFreeSize());
	uint32_t current = _base, len, id, available_id = 0;
	heaphdr_t *h;
	while (current < _heap_size) {
		h = (heaphdr_t*)(_heap + current);
		len = h->len + sizeof(heaphdr_t);
		id = h->id & HEAP_ID_MASK;
		if (id != HEAP_ID_FREE && !heapIdInUse(id, _sb, _fp)) {
			// Дефрагментация
			uint32_t free_len = ((heaphdr_t*)(_heap + _base))->len;
			DEBUGF("HEAP: removing unused object with id 0x%04x (%u bytes)\n", id, len);
			DEBUGF("memmove from %u to %u, length %u\n", _heap + _base + free_len, _heap + _base + len, current - (_base + free_len));
			heap_memcpy_up(_heap + _base + free_len + len, _heap + _base + free_len, current - (_base + free_len));
			// Добавить освобождённую память в свободный блок
			((heaphdr_t*)(_heap + _base))->len += len;
		} else {
			// запомнить ид, чтобы не искать его позже
			if (id > available_id)
				available_id = id;
		}
		current += len;
	}
	if (available_id > _alloc_id)
		_alloc_id = available_id;
	// Ошибка, если какой-то блок был повреждён
	if(current != _heap_size)
		HEAP_ERR(HEAP_ERR_CORRUPTED);
	DEBUGF("After: %u bytes free\n", getFreeSize());
}

// Занимает [length] байт из кучи (для стека)
void Heap::memSteal(uint32_t length) {
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	uint32_t len;
	if (h->id != HEAP_ID_FREE)
		HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	len = h->len;
	// Если нет места
	if(len < length) {
   		garbageCollect(); // Сборка мусора
		len = h->len;
	}
	// Если сборка мусора не освободила память
	if (len < length)
		HEAP_ERR(HEAP_ERR_OVERFLOW); /* Ошибка! Переполнение */
	_base += length;
	h = (heaphdr_t*)(_heap + _base);
	h->id = HEAP_ID_FREE;
	h->len = len - length;
}

// Возвращает [length] байт в кучу
void Heap::memUnsteal(uint32_t length) {
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	uint32_t len;
	if (h->id != HEAP_ID_FREE) {
		HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	}
	if (_base < length) {
		HEAP_ERR(HEAP_ERR_UNDERFLOW); /* Ошибка! Стек пуст */
	}
	len = h->len;
	_base -= length;
	h = (heaphdr_t*)(_heap + _base);
	h->id = HEAP_ID_FREE;
	h->len = len + length;
}