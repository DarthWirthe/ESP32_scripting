
#include "scr_def.h"

//
#define sizeof_lib(l) (sizeof(l) / sizeof(l[0]))

// Временный макрос
#define _esp_get_time() (unsigned long) (esp_timer_get_time() / 1000ULL)

// Временные функции
void print_free_mem() {
	multi_heap_info_t mhi;
	heap_caps_get_info(&mhi, MALLOC_CAP_DEFAULT);
  	PRINTF("Free memory = %u bytes (%u bytes min)\n", 
	  		heap_caps_get_free_size(MALLOC_CAP_DEFAULT), mhi.minimum_free_bytes);
}

void print_occ_mem() {
	multi_heap_info_t mhi;
	heap_caps_get_info(&mhi, MALLOC_CAP_DEFAULT);
	PRINTF("Occupied memory = %u bytes\n", mhi.total_allocated_bytes);
}

//////

LangState::LangState() {
	heapSize_ = 1024;
	parserObj_ = nullptr;
	vm_ = nullptr;
	codeBase_ = nullptr;
	numConsts_ = nullptr;
	funcRef_ = nullptr;
}

LangState::~LangState() {

}

void LangState::setHeapSize(uint32_t size) {
	heapSize_ = constrain(size, HEAP_MIN_SIZE, HEAP_MAX_SIZE);
}

void LangState::registerLib(const char* name, lib_reg *lib, size_t size) {
	lib_struct ls;
	builtinLibs_.push_back(ls);
	builtinLibs_.back().name = name;
	builtinLibs_.back().lib = lib;
	builtinLibs_.back().size = size;
}

void LangState::loadLibs() {
	registerLib("vm", (lib_reg*)lib_native_vm, sizeof_lib(lib_native_vm));
	registerLib("m", (lib_reg*)native_math, sizeof_lib(native_math));
    registerLib("string", (lib_reg*)native_string, sizeof_lib(native_string));
    registerLib(NULL, (lib_reg*)native_convert, sizeof_lib(native_convert));
	registerLib("array", (lib_reg*)native_array, sizeof_lib(native_array));
	registerLib("uart",  (lib_reg*)lib_uart, sizeof_lib(lib_uart));
#ifdef CONFIG_ENABLE_ESP32_LIB
	registerLib("esp", (lib_reg*)lib_esp32, sizeof_lib(lib_esp32));
#endif
#ifdef CONFIG_ENABLE_GPIO_LIB
	registerLib("gpio", (lib_reg*)lib_gpio, sizeof_lib(lib_gpio));
#endif
#ifdef CONFIG_ENABLE_TFT_LIB
	registerLib("tft", (lib_reg*)lib_tft32, sizeof_lib(lib_tft32));
#endif
}

/**
 * @brief Компиляция строки std::string. 
 * Тип ввода: LEX_INPUT_STRING - строка std::string\n
 * LEX_INPUT_FILE_FATFS - файл из файловой системы FAT16/FAT32
 * LEX_INPUT_FILE_SPIFFS - файл из файловой системы SPIFFS
 * @param input_type тип ввода
 * @param s имя файла, либо строка с исходным кодом
 * @return 0 - если успешно, другое число, если обнаружены ошибки
 */
int LangState::compile(int input_type, std::string s) {
	int exit_code;
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	parserObj_ = new Parser(new Lexer(input_type, s));
	unsigned long time1 = _esp_get_time();
	loadLibs();
	parserObj_->compiler()->loadLibs(builtinLibs_);
	builtinLibs_.clear();
	exit_code = parserObj_->parse();
	unsigned long time2 = _esp_get_time() - time1;
	PRINTF("\nВремя компиляции: %u мс\n", time2);
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	if (exit_code == 0) {
		// Временно
		size_t len = parserObj_->getCodeLength();
		size_t consts_count = parserObj_->getConstsCount();
		//
		codeBase_ = parserObj_->getCode();
		numConsts_ = parserObj_->getConsts();
		strConsts_ = parserObj_->compiler()->getStrConsts();
		globalsCount_ = parserObj_->compiler()->getGlobalsCount();
		funcRef_ = GET_CFPT(strConsts_.c_str(), 
				parserObj_->compiler()->getEmbFunctionsCount(),
				parserObj_->compiler()->getFuncReference());
		
		PRINTF("БАЙТКОД [%u] = {\n", len);
		for (size_t i = 0; i < len; i++) {
			PRINTF("%u) %u\n", i, codeBase_[i]);
		}
		PRINTF("}\nКОНСТАНТЫ = {\n");
		for (int n = 0; n < consts_count; n++) {
			PRINTF("%u) %u\n", n, numConsts_[n].i);
		}
		PRINTF("}\n");
	} else {
		PRINTF("Ошибка компиляции, код %u\n", exit_code);
		PRINTF(DEBUG_GET_ERROR_MESSAGE().c_str());
		debug_clear_error_message();
	}

	delete parserObj_;
	parserObj_ = nullptr;
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	return exit_code;
}

/**
 * @brief Компиляция исходного кода в файл. 
 * Тип ввода: LEX_INPUT_STRING - строка std::string, 
 * LEX_INPUT_FILE_FATFS - файл из файловой системы FAT16/FAT32, 
 * LEX_INPUT_FILE_SPIFFS - файл из файловой системы SPIFFS
 * @param input_type тип ввода
 * @param fs файловая система, в которую производится компиляция: SD или SPIFFS
 * @param source имя компилируемого файла, либо строка с исходным кодом
 * @param out_path имя файла, в который производится компиляция
 * @return 0 - если успешно, другое число, если обнаружены ошибки
 */
int LangState::compileToFile(int input_type, fs::FS &fs, std::string source, std::string out_path) {
	File file;
	int exit_code = 0;
	parserObj_ = new Parser(new Lexer(input_type, source));
	loadLibs();
	parserObj_->compiler()->loadLibs(builtinLibs_);
	builtinLibs_.clear();
	exit_code = parserObj_->parse();
	if (exit_code == 0) {
		file = FS_OPEN_FILE(fs, out_path.c_str(), FILE_WRITE);
		parserObj_->compiler()->streamCodeOutput(file);
	} else {
		PRINTF("Ошибка компиляции, код %u\n", exit_code);
		PRINTF(DEBUG_GET_ERROR_MESSAGE().c_str());
		debug_clear_error_message();
	}

	delete parserObj_;
	parserObj_ = nullptr;
	return exit_code;
}

/*
 *
*/
int LangState::run() {
	int vm_exit = 0;
	vm_ = new VM (
		heapSize_, //
		codeBase_,
		numConsts_,
		(char*)strConsts_.c_str(),
		globalsCount_,
		funcRef_
	);
	try {
		vm_exit = vm_->run();
	} catch (int code) {
		vm_exit = code;
	}
	delete vm_;
	vm_ = nullptr;
	PRINTF("\n"); //**//
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	return vm_exit;
}

/* Компиляция строки
 *
*/
int LangState::compileString(std::string s) {
	return compile(LEX_INPUT_STRING, s);
}

/* Компиляция файла
 *
*/
int LangState::compileFile(std::string s) {
	return compile(LEX_INPUT_FILE_SPIFFS, s);
}

int LangState::executeFile(fs::FS &fs, std::string path) {
	int vm_exit = 0;
	if (!fs.exists(path.c_str()))
		return 1;
	File file = fs.open(path.c_str(), FILE_READ);
	if (file) {
		// Файл открыт
		bool success;
		file_meta *data = FS_READ_FILE_META(file, success);
		file.close();
		// Встроенные библиотеки
		loadLibs();
		codeBase_ = (uint8_t*)data->bytecode;
		numConsts_ = data->const_num_reg;
		// В файле может не быть строк
		data->const_str_reg = (data->const_str_reg != nullptr) ? data->const_str_reg : (char*)malloc(1);
		// Получение указателей на функции
		funcRef_ = GET_CFPT(data->const_str_reg, data->function_reg,
			data->func_reg_size / sizeof(uint32_t), builtinLibs_);
		builtinLibs_.clear();
		globalsCount_ = data->globals_count;
		free(data);
		// 
		if (success && funcRef_) {
			vm_ = new VM (
				heapSize_,
				codeBase_,
				numConsts_,
				data->const_str_reg,
				globalsCount_,
				funcRef_
			);
			try {
				vm_exit = vm_->run();
			} catch (int code) {
				vm_exit = code;
			}
			delete vm_;
			vm_ = nullptr;
			return vm_exit;
		}
	}
	return 1;
}
