
#include "scr_def.h"

//
#define sizeof_lib(l) (sizeof((l)) / sizeof((l)[0]))
#define library_content(l) (lib_reg*)(l), sizeof_lib((l))

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
	ls.name = name;
	ls.lib = lib;
	ls.size = size;
	builtinLibs_.push_back(ls);
}

void LangState::loadLibs() {
	registerLib("vm", library_content(lib_native_vm));
	registerLib("m", library_content(native_math));
    registerLib("string", library_content(native_string));
    registerLib(NO_LABEL, library_content(native_convert));
	registerLib("array", library_content(native_array));
	registerLib("uart",  library_content(lib_uart));
#ifdef CONFIG_ENABLE_ESP32_LIB
	registerLib("esp", library_content(lib_esp32));
#endif
#ifdef CONFIG_ENABLE_GPIO_LIB
	registerLib("gpio", library_content(lib_gpio));
#endif
#ifdef CONFIG_ENABLE_TFT_LIB
	registerLib("tft", library_content(lib_tft32));
#endif
#ifdef CONFIG_ENABLE_ESP_VGA_LIB
	registerLib("vga", library_content(lib_esp_vga));
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
int LangState::compile(int input_type, std::string &s) {
	int exitCode;
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	parserObj_ = new Parser(new Lexer(input_type, s));
	loadLibs();
	parserObj_->compiler()->loadLibs(builtinLibs_);
	builtinLibs_.clear();
	exitCode = parserObj_->parse();
	
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	if (exitCode == 0) {
		//
		codeBase_ = parserObj_->getCode();
		numConsts_ = parserObj_->getConsts();
		strConsts_ = *(parserObj_->compiler()->getStrConsts());
		globalsCount_ = parserObj_->compiler()->getGlobalsCount();
		funcRef_ = GET_CFPT(strConsts_.c_str(), 
				parserObj_->compiler()->getEmbFunctionsCount(),
				parserObj_->compiler()->getFuncReference());
	}

	delete parserObj_;
	parserObj_ = nullptr;
	print_free_mem(); //**//
  	print_occ_mem(); //**//
	return exitCode;
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
int LangState::compileToFile(int input_type, fs::FS &fs, std::string &source, std::string &out_path) {
	File file;
	int exitCode = 0;
	parserObj_ = new Parser(new Lexer(input_type, source));
	loadLibs();
	parserObj_->compiler()->loadLibs(builtinLibs_);
	builtinLibs_.clear();
	exitCode = parserObj_->parse();
	if (exitCode == 0) {
		file = FS_OPEN_FILE(fs, out_path.c_str(), FILE_WRITE);
		parserObj_->compiler()->streamCodeOutput(file);
	}

	delete parserObj_;
	parserObj_ = nullptr;
	return exitCode;
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
int LangState::compileString(std::string &s) {
	return compile(LEX_INPUT_STRING, s);
}

/* Компиляция файла
 *
*/
int LangState::compileFile(std::string &s) {
	return compile(LEX_INPUT_FILE_SPIFFS, s);
}

int LangState::executeFile(fs::FS &fs, std::string &path) {
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
		// Получение указателей на функции
		funcRef_ = GET_CFPT(data->const_str_reg.c_str(), data->function_reg,
			data->func_reg_size / sizeof(int), builtinLibs_);
		builtinLibs_.clear();
		globalsCount_ = data->globals_count;
		free(data);
		// 
		if (success && funcRef_) {
			vm_ = new VM (
				heapSize_,
				codeBase_,
				numConsts_,
				(char*)data->const_str_reg.c_str(),
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
