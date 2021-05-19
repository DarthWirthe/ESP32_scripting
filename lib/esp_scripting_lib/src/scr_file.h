
#pragma once
#ifndef SCR_FILE_H
#define SCR_FILE_H

#include <stdint.h>
#include "utils/scr_debug.h"
#include "scr_types.h"
#include "SD.h"
#include "SPIFFS.h"
#include "FS.h"
//#include "utils/crc32/ErriezCRC32.h"

#define FORMAT_SPIFFS_IF_FAILED true

#define SD_CS 5

#define FILE_OUTPUT_SPIFFS 0
#define FILE_OUTPUT_FATFS 1

#define FILE_FIRST_BYTE 0x4F

struct file_header_s {
    uint8_t first_word = FILE_FIRST_BYTE;
    uint8_t major_version;
    uint8_t minor_version;
};

struct file_meta {
    size_t string_reg_size;
    size_t num_reg_size;
    size_t func_reg_size;
    size_t bytecode_size;
    uint32_t globals_count;
    char *const_str_reg;
    num_t *const_num_reg;
    int *function_reg;
    uint8_t *bytecode;
};

bool SPIFFS_BEGIN(void);
bool SD_CARD_BEGIN(void);
File FS_OPEN_FILE(fs::FS &fs, const char *path, const char *mode);
file_meta* FS_READ_FILE_META(File file, bool &success);
void FS_CLOSE_FILE(File file);

#endif // SCR_FILE_H