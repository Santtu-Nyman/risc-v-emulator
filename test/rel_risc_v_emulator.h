#ifndef REL_RISC_V_EMULATOR_H
#define REL_RISC_V_EMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#define REL_ENCODING_X 0
#define REL_ENCODING_R 1
#define REL_ENCODING_I 2
#define REL_ENCODING_S 3
#define REL_ENCODING_B 4
#define REL_ENCODING_U 5
#define REL_ENCODING_J 6
#define REL_ENCODING_I_SHIFT 7
#define REL_ENCODING_I_FENCE 8
#define REL_ENCODING_I_ENVIROMENT 9

#define REL_DISASSEMBLE_NEW_LINE 0x01
#define REL_DISASSEMBLE_ADDRESS 0x02
#define REL_DISASSEMBLE_MACHINE_CODE 0x04
#define REL_DISASSEMBLE_ENCODING 0x08
#define REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS 0x10
#define REL_DISASSEMBLE_USE_PSEUDOINSTRUCTIONS 0x20

#define REL_REGISTER_CONTEXT_GENERAL 0
#define REL_REGISTER_CONTEXT_PC 1

typedef struct rel32i_register_set_t
{
	uint32_t pc;
	uint32_t x1_x31[31];
} rel32i_register_set_t;

typedef struct rel32_instruction_information_t
{
	uint32_t machine_code;
	const char* mnemonic;
	const char* module;
	int instruction_index;
	uint8_t size;
	uint8_t encoding;
	uint8_t opcode;
	uint8_t rd;
	uint8_t function3;
	uint8_t rs1;
	uint8_t rs2;
	uint8_t function7;
	uint8_t compressed_function6;
	uint8_t compressed_function4;
	uint8_t compressed_function3;
	uint32_t intermediate;
} rel32_instruction_information_t;

void rel32_copy(void* destination, const void* source, size_t size);

size_t rel32_string_size(const char* string);

size_t rel32_string_line_size(const char* string);

size_t rel32_get_string_line_offset(const char* string, size_t line_number);

size_t rel32_get_string_line_count(const char* string);

int rel32_string_compare(const char* a, const char* b);

size_t rel32_print_hex(uint32_t value, char* buffer);

size_t rel32_print_hex_digits(int digit_count, uint32_t value, char* buffer);

size_t rel32_print_unsigned(uint32_t value, char* buffer);

size_t rel32_print_signed(int32_t value, char* buffer);

int rel32_print_instruction_encoding_format(const char* mnemonic, char* buffer);

int rel32_print_instruction_encoding(const char* mnemonic, char* buffer);

void rel32_decode_instruction(const void* address_of_instruction, rel32_instruction_information_t* information_information);

int rel32_get_register_name(int context, int number, int use_abi_name, char** pointer_to_name_pointer, size_t* pointer_name_size);

int rel32_disassemble_instruction(int flags, const void* base_address, uint32_t address_of_instruction, size_t assembly_buffer_size, size_t* assembly_size, char* assembly_buffer);

void rel32i_step_instruction(const void* code_base_address, void* data_base_address, rel32i_register_set_t* register_set);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // REL_RISC_V_EMULATOR_H
