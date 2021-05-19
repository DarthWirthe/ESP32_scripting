/**
 * @file scr_def.h
 * @brief API для использования основных функций языка программирования и виртуальной машины
 * предоставляет возможности компилятора для мгновенного выполнения исходного кода,
 * создания исполняемых файлов и их выполнения.
**/

#pragma once
#ifndef SCR_DEF_H
#define SCR_DEF_H

#include "libs/include_libs.h"
#include "scr_parser.h"
#include "scr_vm.h"

class LangState {
	public:
		LangState(void);
		~LangState();
		void setHeapSize(uint32_t size);
		void registerLib(const char* name, lib_reg *lib, size_t size);
		void loadLibs(void);
		int compile(int input_type, std::string s);
		int compileToFile(int input_type, fs::FS &fs, std::string source, std::string out_path);
		int compileString(std::string s);
		int compileFile(std::string s);
		int executeFile(fs::FS &fs, std::string path);
		int run(void);
	private:
		Parser *parserObj_;
		VM *vm_;
		std::vector<lib_struct> builtinLibs_;
		uint8_t *codeBase_;
		std::string strConsts_;
		num_t *numConsts_;
		cfunction *funcRef_;
		unsigned int globalsCount_;
		unsigned int heapSize_;
};


#endif