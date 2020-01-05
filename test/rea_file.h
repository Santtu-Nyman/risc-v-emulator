#ifndef REL_RISC_V_APPLICATION_FILE_H
#define REL_RISC_V_APPLICATION_FILE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "rel_risc_v_emulator.h"

#define REA_IGNORE_DIRECTORY 0
#define REA_WORKING_DIRECTORY 1
#define REA_PROGRAM_DIRECTORY 2
#define REA_SPECIAL_DIRECTORY_COUNT 3

typedef struct rel32_binary_t
{
	char* file_name;
	size_t size;
	size_t instruction_count;
	uint32_t* data;
	size_t disassembly_length;
	char* disassembly;
} rel32_binary_t;

int rea_get_special_directory_path(int special_directory, size_t path_buffer_size, size_t* path_size, char* path_buffer);

int rea_load_file(int special_directory, const char* file_name, size_t file_data_buffer_size, size_t* file_size, void* file_data_buffer);

int rea_store_file(int special_directory, const char* file_name, size_t file_size, void* file_data_buffer);

int rea32_load_binary_file(int special_directory, const char* file_name, rel32_binary_t** pointer_to_binary);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // REL_RISC_V_APPLICATION_FILE_H
