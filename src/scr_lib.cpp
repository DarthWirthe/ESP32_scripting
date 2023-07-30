#include "scr_lib.h"

static Heap *heap_reference = nullptr;

static bool SDEnabledFlag = false;
static bool SPIFFSEnabledFlag = false;

void VMInternalException() {
    throw VM_FUNCTION_EXCEPTION;
}

void set_heap_ref(Heap *ref) {
    heap_reference = ref;
}

Heap* heap_ref() {
    return heap_reference;
}

// Добавить строку в виртуальную память
uint32_t VM_LoadString(Heap *heap, char *c) {
	size_t length = strlen(c);
	uint32_t id = heap->alloc(length + 1);
	char *pointer = (char*)heap->getAddr(id);
	memcpy(pointer, c, length);
	*(pointer + length) = 0;
	return id | TYPE_HEAP_PT_MASK;
}

void setSDEnabledFlag(bool v) {
    SDEnabledFlag = v;
}

void setSPIFFSEnabledFlag(bool v) {
    SPIFFSEnabledFlag = v;
}

bool isSDEnabled(void) {
    return SDEnabledFlag;
}

bool isSPIFFSEnabled(void) {
    return SPIFFSEnabledFlag;
}

static cfunction* _return_cfpt(std::vector<cfunction> pts) {
    size_t size = (pts.size() > 0) ? pts.size() : 1;
    cfunction *ret = (cfunction*)malloc(sizeof(cfunction) * size);
    if (!ret)
        return nullptr;
    int n = 0;
    for (auto i = pts.begin(); i < pts.end(); i++) {
        ret[n] = *i;
        n++;
    }
    return ret;
}

// Возвращает указатели на встроенные функции для вм (версия для загрузки из файла)
cfunction* GET_CFPT(const char *src, int *function_reg, size_t len, std::vector<lib_struct> libs) {
    std::vector<cfunction> f_pts;
    std::string temp;
    char *lib_label, *func_name;
    if (len > 0 && src == nullptr)
        return nullptr;

    for (int i = 0; i < len; i++) {

        lib_label = nullptr;
        func_name = nullptr;
        // Определить имя функции
        temp = src + function_reg[i];
        size_t dot = temp.find('.');
        if (dot != std::string::npos) {
            lib_label = (char*)temp.substr(0, dot).c_str();
            func_name = (char*)temp.substr(dot + 1, temp.length()).c_str();
        } else {
            func_name = (char*)temp.c_str();
        }
        // Поиск функции
        for (auto n = libs.begin(); n != libs.end(); n++) {
            // если найдена библиотека с такой меткой
            if ((!n->name && !lib_label) || 
                (lib_label && n->name && compareStrings(lib_label, n->name))) {
                for (int l = 0; l < n->size; l++) {
                    if (compareStrings(n->lib[l].name, func_name)) {
                        f_pts.push_back(n->lib[l].func);
                        goto function_found; // Функция найдена
                    }
                }
            }
        }
        return nullptr; // Функция не найдена
function_found:
        continue;
    }

    return _return_cfpt(f_pts);
}

// Возвращает указатели на встроенные функции для вм
cfunction* GET_CFPT(const char *src, size_t count, std::vector<cfunction> f_pts) {
    return _return_cfpt(f_pts);
}
