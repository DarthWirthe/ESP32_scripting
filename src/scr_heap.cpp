/* scr_heap.cpp
** Виртуальная память
*/

#include "scr_heap.h"

#ifdef VM_USE_FIXED_MEMORY
#if VM_DEFAULT_HEAP_SIZE > 102400
#error "VM_DEFAULT_HEAP_SIZE must be <= 102400"
#endif
heap_t _heap[VM_DEFAULT_HEAP_SIZE];
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
	_heap_index = (heapindex_t*)malloc(sizeof(heapindex_t) * HEAP_INDEX_SIZE);
	if (!_heap_index)
		HEAP_ERR(HEAP_ERR_OUT_OF_RAM); // Недостаточно памяти ОЗУ
	// 'heap_index' надо заполнить нулями:
	memset((uint8_t*)_heap_index, 0x00, sizeof(heapindex_t) * HEAP_INDEX_SIZE);
	
	_base = 0;
	// Один пустой блок занимает всё пространство
	h = (heaphdr_t*)&_heap[0];
	_heap_index[0].addr = 0;
	_heap_index[0].type = 0;
	//h->id = HEAP_ID_FREE;
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
	free(_heap_index);
}


void Heap::heapDump() {
	PRINTF("HEAP: DUMP()\n");
	for (int id = 1; id < HEAP_INDEX_SIZE; id++) {
		heaphdr_t *h = (heaphdr_t*)(_heap + _heap_index[id].addr);
		uint8_t *begin = (uint8_t*)(h + 1);
		uint16_t len = h->len + sizeof(heaphdr_t);
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
static inline heaphdr_t* heap_search(heap_t *heap, heapindex_t *heap_index, u16_t id) {
	if (heap_index[id].addr) {
		return (heaphdr_t*)(heap + heap_index[id].addr);
	}
	return 0;
}

// Поиск свободного блока
u16_t Heap::heapNewId() {
	for (int id = 1; id < HEAP_INDEX_SIZE; id++) {
		if (!_heap_index[id].addr)
			return id;
	}
	return 0;
}

// Выделение памяти по идентификатору
bool Heap::heapAllocInternal(u16_t id, heap_size_t size) {
	uint16_t required = size + sizeof(heaphdr_t);
	uint16_t shift;
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	if (h->len >= required) { // если есть свободное пространство
		h->len -= required; // уменьшение свободного блока
		shift = _base + sizeof(heaphdr_t) + h->len;
		h = (heaphdr_t*)(_heap + shift);
		_heap_index[id].addr = shift;
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
		id = heapNewId();
		if (!id)
			HEAP_ERR(HEAP_ERR_OUT_OF_IDS); /* Закончились номерки */
	}
	if (!heapAllocInternal(id, size)) {
		// Если нет свободного пространства:
		garbageCollect(); // сборка мусора
		//id = heap_new_id(); // ид был переопределён
		if (!id) {
			id = heapNewId();
			if (!id)
				HEAP_ERR(HEAP_ERR_OUT_OF_IDS);
		}
		if (!heapAllocInternal(id, size)) // попробовать снова
			HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	}
	DEBUGF("Heap::alloc(): id=%u, size=%u\n", id, size);
	return id;
}

// Перераспределение блока
void Heap::realloc(u16_t id, heap_size_t size) {
	heaphdr_t *h, *h_new;
	// заголовок свободного блока
	h = (heaphdr_t*)(_heap + _base);
	if (h->len >= size + sizeof(heaphdr_t))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	// заголовок блока
	h = heap_search(_heap, _heap_index, id);
	// создание нового блока
	if (!heapAllocInternal(id, size))
		HEAP_ERR(HEAP_ERR_OUT_OF_MEMORY); /* Ошибка! Нет места */
	h_new = heap_search(_heap, _heap_index, id);
	memcpy(h_new + 1, h + 1, h->len);
	// Старый блок будет удалён сборщиком
	_heap_index[id].addr = HEAP_ID_FREE;
}

// Возвращает длину блока
heap_size_t Heap::getLength(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _heap_index, id);
	if (!h) {
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	}
	return h->len;
}

// Возвращает адрес данных в блоке
void* Heap::getAddr(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _heap_index, id);
	if (!h)
		HEAP_ERR(HEAP_ERR_NOT_FOUND); /* Ошибка! Не найден блок */
	return h + 1;
}

// Возвращает адрес заголовка блока
heaphdr_t* Heap::getHeader(u16_t id) {
	heaphdr_t *h = heap_search(_heap, _heap_index, id);
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

// Возвращает размер кучи
uint32_t Heap::getHeapSize() {
	return _heap_size - _base;
}

// Возвращает количество блоков
uint32_t Heap::getBlocksCount() {
	uint32_t count = 0;
	for (int id = 1; id < HEAP_INDEX_SIZE; id++) {
		if (_heap_index[id].addr)
			count++;
	}
	return count;
}

// Возвращает указатель на начало свободного блока
heap_t* Heap::getRelativeBase() {
	return _heap + _base;
}

heap_t* Heap::getBase() {
#ifdef VM_USE_FIXED_MEMORY
	return _heap;
#else
	return _heap;
#endif
}

bool heapIdInUse(uint16_t id, stack_t *base, stack_t *fp) {
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
	// поиск в глобальных переменных
	//PRINTF("##PT=%u,BASE=%u\n",pt,_base);
	for (;pt != base; pt--) {
		if (*pt == ref_id)
			return true;
	}
	return false;
}

/* Функция сборки мусора.
 * Проверяет каждый блок в куче на достижимость:
 * если объект не используется, то он удаляется из памяти
*/
size_t Heap::garbageCollect() {
	DEBUGF("HEAP: garbageCollect()\nBefore: %u bytes free\n", getFreeSize());
	uint32_t current = _base, len, id;// free_id = 0;
	uint32_t freed_mem = 0;
	heaphdr_t *h, *baseh = (heaphdr_t*)(_heap + _base);
	uint32_t free_len = baseh->len;

	for (int id = 1; id < HEAP_INDEX_SIZE; id++) {
		h = (heaphdr_t*)(_heap + _heap_index[id].addr);
		len = h->len + sizeof(heaphdr_t);

		if (id != HEAP_ID_FREE && !heapIdInUse(id, _sb, _fp)) {
			// Дефрагментация
			uint8_t *block_pos = _heap + _base + free_len;
			DEBUGF("HEAP: removing unused object with id 0x%04x (%u bytes)\n", id, len);
			DEBUGF("memmove from %u to %u, length %u\n", block_pos, block_pos + len, _heap_index[id].addr - (_base + free_len));
			_heap_index[id].addr = _heap_index[id].addr + len; // сдвигается на len байт вправо
			heap_memcpy_up(block_pos + len, block_pos, _heap_index[id].addr - (_base + free_len));
			// Добавить освобождённую память в свободный блок
			freed_mem += len;
			free_len += len;
		}

		current += len;
	}

	baseh->len += freed_mem;
	
	// Ошибка, если какой-то блок имеет неверный размер
	if (current != _heap_size)
		HEAP_ERR(HEAP_ERR_CORRUPTED);
	DEBUGF("After: %u bytes free\n", getFreeSize());
	//PRINTF("HEAP::garbageCollect: alloc_id = %u, end = %u\n", _alloc_id, _ids_unused_end);
	return freed_mem;
}

// Занимает [length] байт из кучи (для стека)
void Heap::memSteal(uint32_t length) {
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	uint32_t len;
	//if (h->id != HEAP_ID_FREE)
	//	HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	len = h->len;
	// Если нет места
	if (len < length) {
   		garbageCollect(); // Сборка мусора
		len = h->len;
	}
	// Если сборка мусора не освободила память
	if (len < length)
		HEAP_ERR(HEAP_ERR_OVERFLOW); /* Ошибка! Переполнение */
	_base += length;
	h = (heaphdr_t*)(_heap + _base);
	//h->id = HEAP_ID_FREE;
	h->len = len - length;
}

// Возвращает [length] байт в кучу
void Heap::memUnsteal(uint32_t length) {
	heaphdr_t *h = (heaphdr_t*)(_heap + _base);
	uint32_t len;
	//if (h->id != HEAP_ID_FREE) {
	//	HEAP_ERR(HEAP_ERR_CORRUPTED); /* Ошибка! Куча повреждена */
	//}
	if (_base < length) {
		HEAP_ERR(HEAP_ERR_UNDERFLOW); /* Ошибка! Стек пуст */
	}
	len = h->len;
	_base -= length;
	h = (heaphdr_t*)(_heap + _base);
	//h->id = HEAP_ID_FREE;
	h->len = len + length;
}