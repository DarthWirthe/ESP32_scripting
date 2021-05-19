
#include "scr_file.h"

bool SPIFFS_BEGIN() {
	return SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
}

bool SD_CARD_BEGIN() {
    return SD.begin(SD_CS);
}

File FS_OPEN_FILE(fs::FS &fs, const char *path, const char *mode) {
	File file = fs.open(path, mode);
	if (!file || file.isDirectory()) {
        PRINTF("- failed to open file for reading");
    }
	return file;
}

static uint32_t Combine_8_to_32u(char *b) {
	return b[3] << 24 | b[2] << 16 | b[1] << 8 | b[0];
}

file_meta* FS_READ_FILE_META(File file, bool &success) {
	PRINTF("\nFS_READ_FILE_META\n");
	file_meta *fm = new file_meta();
	file_header_s file_hdr;
	char buf4b[4];

	if (!file || !file.available()) {
		success = false;
		return fm;
	}

	PRINTF("file size = %u bytes\n", file.size());
	// Файл начинается с байта 0x4F
	file.readBytes((char*)&file_hdr, sizeof(file_hdr));
	if (file_hdr.first_word != FILE_FIRST_BYTE) {
		success = false;
		return fm;
	}
	PRINTF("compiler version: %u.%u\n", file_hdr.major_version, file_hdr.minor_version);

	file.readBytes(buf4b, 4);
	fm->string_reg_size = Combine_8_to_32u(buf4b);
	PRINTF("string_reg_size %u bytes\n", fm->string_reg_size);
	fm->const_str_reg = (char*)malloc(sizeof(char) * fm->string_reg_size);
	file.readBytes(fm->const_str_reg, fm->string_reg_size);

	file.readBytes(buf4b, 4);
	fm->globals_count = Combine_8_to_32u(buf4b);
	PRINTF("globals count: %u\n", fm->globals_count);

	file.readBytes(buf4b, 4);
	int num_contst_count = Combine_8_to_32u(buf4b);
	fm->num_reg_size = num_contst_count * 4;
	PRINTF("num_reg_size %u bytes\n", fm->num_reg_size);
	fm->const_num_reg = (num_t*)malloc(sizeof(num_t) * num_contst_count);
	file.readBytes((char*)fm->const_num_reg, fm->num_reg_size);

	file.readBytes(buf4b, 4);
	int func_refs_count = Combine_8_to_32u(buf4b);
	fm->func_reg_size = func_refs_count * 4;
	PRINTF("func_reg_size %u bytes\n", fm->func_reg_size);
	fm->function_reg = (int*)malloc(sizeof(int) * func_refs_count);
	file.readBytes((char*)fm->function_reg, fm->func_reg_size);

	file.readBytes(buf4b, 4);
	fm->bytecode_size = Combine_8_to_32u(buf4b);
	PRINTF("bytecode_size %u bytes\n", fm->bytecode_size);
	fm->bytecode = (uint8_t*)malloc(sizeof(uint8_t) * fm->bytecode_size);
	file.readBytes((char*)fm->bytecode, fm->bytecode_size);

	success = true;
	return fm;
}

void FS_CLOSE_FILE(File file) {
	file.close();
}