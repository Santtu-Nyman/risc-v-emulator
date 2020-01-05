#include "rel_risc_v_emulator.h"
#include <assert.h>

static const struct
{
	const char* mnemonic;
	const char* module;
	const char* encoding_string;
	size_t size;
	uint32_t constant_mask;
	uint32_t constant;
	uint8_t machine_encoding;
	uint8_t assembly_encoding;
	uint8_t opcode;
	uint8_t function3;
	uint8_t function7; }
		instruction_table[] = {
			{ "lui", "i", "imm[31:12],rd,0110111", 4, 0x0000007F, 0x00000037, REL_ENCODING_U, REL_ENCODING_U, 0x37, 0, 0 },
			{ "auipc", "i", "imm[31:12],rd,0010111", 4, 0x0000007F, 0x00000017, REL_ENCODING_U, REL_ENCODING_U, 0x17, 0, 0 },
			{ "jal", "i", "imm[20|10:1|11|19:12],rd,1101111", 4, 0x0000007F, 0x0000006F, REL_ENCODING_J, REL_ENCODING_J, 0x6F, 0, 0 },
			{ "jalr", "i", "imm[11:0],rs1,000,rd,1100111", 4, 0x0000707F, 0x00000067, REL_ENCODING_I, REL_ENCODING_I, 0x67, 0x0, 0 },
			{ "beq", "i", "imm[12|10:5],rs2,rs1,000,imm[4:1|11],1100011", 4, 0x0000707F, 0x00000063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x0, 0 },
			{ "bne", "i", "imm[12|10:5],rs2,rs1,001,imm[4:1|11],1100011", 4, 0x0000707F, 0x00001063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x1, 0 },
			{ "blt", "i", "imm[12|10:5],rs2,rs1,100,imm[4:1|11],1100011", 4, 0x0000707F, 0x00004063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x4, 0 },
			{ "bge", "i", "imm[12|10:5],rs2,rs1,101,imm[4:1|11],1100011", 4, 0x0000707F, 0x00005063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x5, 0 },
			{ "bltu", "i", "imm[12|10:5],rs2,rs1,110,imm[4:1|11],1100011", 4, 0x0000707F, 0x00006063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x6, 0 },
			{ "bgeu", "i", "imm[12|10:5],rs2,rs1,111,imm[4:1|11],1100011", 4, 0x0000707F, 0x00007063, REL_ENCODING_B, REL_ENCODING_B, 0x63, 0x7, 0 },
			{ "lb", "i", "imm[11:0],rs1,000,rd,0000011", 4, 0x0000707F, 0x00000003, REL_ENCODING_I, REL_ENCODING_I, 0x03, 0x0, 0 },
			{ "lh", "i", "imm[11:0],rs1,001,rd,0000011", 4, 0x0000707F, 0x00001003, REL_ENCODING_I, REL_ENCODING_I, 0x03, 0x1, 0 },
			{ "lw", "i", "imm[11:0],rs1,010,rd,0000011", 4, 0x0000707F, 0x00002003, REL_ENCODING_I, REL_ENCODING_I, 0x03, 0x2, 0 },
			{ "lbu", "i", "imm[11:0],rs1,100,rd,0000011", 4, 0x0000707F, 0x00004003, REL_ENCODING_I, REL_ENCODING_I, 0x03, 0x4, 0 },
			{ "lhu", "i", "imm[11:0],rs1,101,rd,0000011", 4, 0x0000707F, 0x00005003, REL_ENCODING_I, REL_ENCODING_I, 0x03, 0x5, 0 },
			{ "sb", "i", "imm[11:5],rs2,rs1,000,imm[4:0],0100011", 4, 0x0000707F, 0x00000023, REL_ENCODING_S, REL_ENCODING_S, 0x23, 0x0, 0 },
			{ "sh", "i", "imm[11:5],rs2,rs1,001,imm[4:0],0100011", 4, 0x0000707F, 0x00001023, REL_ENCODING_S, REL_ENCODING_S, 0x23, 0x1, 0 },
			{ "sw", "i", "imm[11:5],rs2,rs1,010,imm[4:0],0100011", 4, 0x0000707F, 0x00002023, REL_ENCODING_S, REL_ENCODING_S, 0x23, 0x2, 0 },
			{ "addi", "i", "imm[11:0],rs1,000,rd,0010011", 4, 0x0000707F, 0x00000013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x0, 0 },
			{ "slti", "i", "imm[11:0],rs1,010,rd,0010011", 4, 0x0000707F, 0x00002013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x2, 0 },
			{ "sltiu", "i", "imm[11:0],rs1,011,rd,0010011", 4, 0x0000707F, 0x00003013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x3, 0 },
			{ "xori", "i", "imm[11:0],rs1,100,rd,0010011", 4, 0x0000707F, 0x00004013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x4, 0 },
			{ "ori", "i", "imm[11:0],rs1,110,rd,0010011", 4, 0x0000707F, 0x00006013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x6, 0 },
			{ "andi", "i", "imm[11:0],rs1,111,rd,0010011", 4, 0x0000707F, 0x00007013, REL_ENCODING_I, REL_ENCODING_I, 0x13, 0x7, 0 },
			{ "slli", "i", "0000000,shamt,rs1,001,rd,0010011", 4, 0xFE00707F, 0x00001013, REL_ENCODING_I, REL_ENCODING_I_SHIFT, 0x13, 0x1, 0x00 },
			{ "srli", "i", "0000000,shamt,rs1,101,rd,0010011", 4, 0xFE00707F, 0x00005013, REL_ENCODING_I, REL_ENCODING_I_SHIFT, 0x13, 0x5, 0x00 },
			{ "srai", "i", "0100000,shamt,rs1,101,rd,0010011", 4, 0xFE00707F, 0x40005013, REL_ENCODING_I, REL_ENCODING_I_SHIFT, 0x13, 0x5, 0x20 },
			{ "add", "i", "0000000,rs2,rs1,000,rd,0110011", 4, 0xFE00707F, 0x00000033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x0, 0x00 },
			{ "sub", "i", "0100000,rs2,rs1,000,rd,0110011", 4, 0xFE00707F, 0x40000033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x0, 0x20 },
			{ "sll", "i", "0000000,rs2,rs1,001,rd,0110011", 4, 0xFE00707F, 0x00001033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x1, 0x00 },
			{ "slt", "i", "0000000,rs2,rs1,010,rd,0110011", 4, 0xFE00707F, 0x00002033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x2, 0x00 },
			{ "sltu", "i", "0000000,rs2,rs1,011,rd,0110011", 4, 0xFE00707F, 0x00003033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x3, 0x00 },
			{ "xor", "i", "0000000,rs2,rs1,100,rd,0110011", 4, 0xFE00707F, 0x00004033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x4, 0x00 },
			{ "srl", "i", "0000000,rs2,rs1,101,rd,0110011", 4, 0xFE00707F, 0x00005033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x5, 0x00 },
			{ "sra", "i", "0100000,rs2,rs1,101,rd,0110011", 4, 0xFE00707F, 0x40005033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x5, 0x20 },
			{ "or", "i", "0000000,rs2,rs1,110,rd,0110011", 4, 0xFE00707F, 0x00006033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x6, 0x00 },
			{ "and", "i", "0000000,rs2,rs1,111,rd,0110011", 4, 0xFE00707F, 0x00007033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x7, 0x00 },
			{ "fence", "i", "fm,pred,succ,rs1,000,rd,0001111", 4, 0x0000707F, 0x0000000F, REL_ENCODING_I, REL_ENCODING_I_FENCE, 0x0F, 0x0, 0 },
			{ "ecall", "i", "000000000000,00000,000,00000,1110011", 4, 0xFFFFFFFF, 0x00000073, REL_ENCODING_I, REL_ENCODING_I_ENVIROMENT, 0x73, 0x0, 0x00 },
			{ "ebreak", "i", "000000000001,00000,000,00000,1110011", 4, 0xFFFFFFFF, 0x00100073, REL_ENCODING_I, REL_ENCODING_I_ENVIROMENT, 0x73, 0x0, 0x00 },
			{ "fence.i", "zifencei", "imm[11:0],rs1,001,rd,0001111", 4, 0x0000707F, 0x0000100F, REL_ENCODING_I, REL_ENCODING_I, 0x0F, 0x1, 0 },
			{ "csrrw", "zcsr", "csr,rs1,001,rd,1110011", 4, 0x0000707F, 0x00001073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x1, 0 },
			{ "csrrs", "zcsr", "csr,rs1,010,rd,1110011", 4, 0x0000707F, 0x00002073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x2, 0 },
			{ "csrrc", "zcsr", "csr,rs1,011,rd,1110011", 4, 0x0000707F, 0x00003073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x3, 0 },
			{ "csrrwi", "zcsr", "csr,uimm,101,rd,1110011", 4, 0x0000707F, 0x00005073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x5, 0 },
			{ "csrrsi", "zcsr", "csr,uimm,110,rd,1110011", 4, 0x0000707F, 0x00006073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x6, 0 },
			{ "csrrci", "zcsr", "csr,uimm,111,rd,1110011", 4, 0x0000707F, 0x00007073, REL_ENCODING_I, REL_ENCODING_I, 0x73, 0x7, 0 },
			{ "mul", "m", "0000001,rs2,rs1,000,rd,0110011", 4, 0xFE00707F, 0x02000033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x0, 0x01 },
			{ "mulh", "m", "0000001,rs2,rs1,001,rd,0110011", 4, 0xFE00707F, 0x02001033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x1, 0x01 },
			{ "mulhsu", "m", "0000001,rs2,rs1,010,rd,0110011", 4, 0xFE00707F, 0x02002033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x2, 0x01 },
			{ "mulhu", "m", "0000001,rs2,rs1,011,rd,0110011", 4, 0xFE00707F, 0x02003033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x3, 0x01 },
			{ "div", "m", "0000001,rs2,rs1,100,rd,0110011", 4, 0xFE00707F, 0x02004033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x4, 0x01 },
			{ "divu", "m", "0000001,rs2,rs1,101,rd,0110011", 4, 0xFE00707F, 0x02005033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x5, 0x01 },
			{ "rem", "m", "0000001,rs2,rs1,110,rd,0110011", 4, 0xFE00707F, 0x02006033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x6, 0x01 },
			{ "remu", "m", "0000001,rs2,rs1,111,rd,0110011", 4, 0xFE00707F, 0x02007033, REL_ENCODING_R, REL_ENCODING_R, 0x33, 0x7, 0x01 },
			{ "lr.w", "a", "00010,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x1000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x10 },
			{ "sc.w", "a", "00011,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x1800202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x18 },
			{ "amoswap.w", "a", "00001,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x0800202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x08 },
			{ "amoadd.w", "a", "00000,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x0000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x00 },
			{ "amoxor.w", "a", "00100,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x2000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x20 },
			{ "amoand.w", "a", "01100,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x6000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x60 },
			{ "amoor.w", "a", "01000,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x4000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x40 },
			{ "amomin.w", "a", "10000,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0x8000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0x80 },
			{ "amomax.w", "a", "10100,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0xA000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0xA0 },
			{ "amominu.w", "a", "11000,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0xC000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0xC0 },
			{ "amomaxu.w", "a", "11100,aq,rl,rs2,rs1,010,rd,0101111", 4, 0xF800707F, 0xE000202F, REL_ENCODING_R, REL_ENCODING_R, 0x2F, 0x2, 0xE0 } };

void rel32_copy(void* destination, const void* source, size_t size)
{
	for (const void* source_end = (const void*)((uintptr_t)source + size); source != source_end; source = (const void*)((uintptr_t)source + 1), destination = (void*)((uintptr_t)destination + 1))
		*(uint8_t*)destination = *(const uint8_t*)source;
}

size_t rel32_string_size(const char* string)
{
	const char* read_string = string;
	while (*read_string)
		++read_string;
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t rel32_string_line_size(const char* string)
{
	const char* read_string = string;
	while (*read_string && *read_string != '\n' && *read_string != '\r')
		++read_string;
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t rel32_get_string_line_offset(const char* string, size_t line_number)
{
	const char* read_string = string;
	for (size_t line_index = 0; line_index != line_number; ++line_index)
	{
		read_string += rel32_string_line_size(read_string);
		if (!*read_string)
			return (size_t)~0;
		++read_string;
	}
	return (size_t)((uintptr_t)read_string - (uintptr_t)string);
}

size_t rel32_get_string_line_count(const char* string)
{
	size_t line_count = 1;
	while (*string)
	{
		if (*string == '\n' || *string == '\r')
			++line_count;
		++string;
	}
	return line_count;
}

int rel32_string_compare(const char* a, const char* b)
{
	char a_tmp = *a++;
	char b_tmp = *b++;
	while (a_tmp != 0)
	{
		if (a_tmp != b_tmp)
			return 0;
		a_tmp = *a++;
		b_tmp = *b++;
	}
	return !b_tmp;
}
	
size_t rel32_print_hex(uint32_t value, char* buffer)
{
	static const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = 0; i != 8; ++i)
		buffer[i] = hex_table[(value >> ((7 - i) << 2)) & 0xF];
	return 8;
}

size_t rel32_print_hex_digits(int digit_count, uint32_t value, char* buffer)
{
	static const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	
	int fill = 0;
	while (digit_count > 8)
		buffer[fill++] = hex_table[0];

	for (int b = 8 - digit_count, i = b; i != 8; ++i)
		buffer[fill + i - b] = hex_table[(value >> ((7 - i) << 2)) & 0xF];

	return (size_t)digit_count;
}

size_t rel32_print_unsigned(uint32_t value, char* buffer)
{
	size_t size = 0;
	do
	{
		for (char* move = buffer + size++; move != buffer; --move)
			*move = *(move - 1);
		*buffer = '0' + (char)(value % 10);
		value /= 10;
	} while (value);
	return size;// max size is 10
}

size_t rel32_print_signed(int32_t value, char* buffer)
{
	int is_negative = value < 0;
	if (is_negative)
		*buffer = '-';
	return (size_t)is_negative + rel32_print_unsigned((uint32_t)(is_negative ? -value : value), buffer + is_negative);// max size is 11
}

int rel32_print_instruction_encoding_format(const char* mnemonic, char* buffer)
{
	size_t i = 0;
	while (i != (sizeof(instruction_table) / sizeof(*instruction_table)) && !rel32_string_compare(mnemonic, instruction_table[i].mnemonic))
		++i;

	if (i == (sizeof(instruction_table) / sizeof(*instruction_table)))
		return ENOENT;

	static const char* encoding_format_table[] = {
		"x", "| 31 ignored 0 |",
		"r", "| 31 funct7 25 | 24 rs2 20 | 19 rs1 15 | 14 funct3 12 | 11 rd 7 | 6 opcode 0 |",
		"i", "| 31 imm[11:0] 20 | 19 rs1 15 | 14 funct3 12 | 11 rd 7 | 6 opcode 0 |",
		"s", "| 31 imm[11:5] 25 | 24 rs2 20 | 19 rs1 15 | 14 funct3 12 | 11 imm[4:0] 7 | 6 opcode 0 |",
		"b", "| 31 imm[12|10:5] 25 | 24 rs2 20 | 19 rs1 15 | 14 funct3 12 | 11 imm[4:1|11] 7 | 6 opcode 0 |",
		"u", "| 31 imm[31:12] 12 | 11 rd 7 | 6 opcode 0 |",
		"j", "| 31 imm[20|10:1|11|19:12] 12 | 11 rd 7 | 6 opcode 0 |" };

	// max size is 94
	rel32_copy(buffer, encoding_format_table[(int)instruction_table[i].machine_encoding * 2 + 1], rel32_string_size(encoding_format_table[(int)instruction_table[i].machine_encoding * 2 + 1]) + 1);

	return 0;
}

int rel32_print_instruction_encoding(const char* mnemonic, char* buffer)
{
	size_t i = 0;
	while (i != (sizeof(instruction_table) / sizeof(*instruction_table)) && !rel32_string_compare(mnemonic, instruction_table[i].mnemonic))
		++i;

	if (i == (sizeof(instruction_table) / sizeof(*instruction_table)))
		return ENOENT;

	rel32_copy(buffer, instruction_table[i].encoding_string, rel32_string_size(instruction_table[i].encoding_string) + 1);

	return 0;
}

void rel32_decode_instruction(const void* address_of_instruction, rel32_instruction_information_t* information_information)
{
	uint32_t instruction = ((uint32_t)*(uint8_t*)((uintptr_t)address_of_instruction + 0) << 0) | ((uint32_t)*(uint8_t*)((uintptr_t)address_of_instruction + 1) << 8);
	int instruction_is_compressed = (instruction & 0x0003) != 0x0003;
	if (instruction_is_compressed)
	{
		information_information->size = 2;
		information_information->encoding = 0;
		information_information->opcode = (instruction >> 0) & 0x0003;
		information_information->rd = 0;
		information_information->function3 = 0;
		information_information->rs1 = 0;
		information_information->rs2 = 0;
		information_information->function7 = 0;
		information_information->compressed_function6 = 0;
		information_information->compressed_function4 = 0;
		information_information->compressed_function3 = 0;

		information_information->intermediate = 0;
	}
	else
	{
		instruction |= ((uint32_t)*(uint8_t*)((uintptr_t)address_of_instruction + 2) << 16) | ((uint32_t)*(uint8_t*)((uintptr_t)address_of_instruction + 3) << 24);

		information_information->size = 4;
		information_information->encoding =
			((((instruction & 0x0000007F) == 0x33) || ((instruction & 0x0000007F) == 0x2F)) ? REL_ENCODING_R : 0) |
			((((instruction & 0x0000007F) == 0x67) || ((instruction & 0x0000007F) == 0x73) || ((instruction & 0x0000007F) == 0x0F) || ((instruction & 0x0000007F) == 0x03) || ((instruction & 0x0000007F) == 0x13)) ? REL_ENCODING_I : 0) |
			(((instruction & 0x0000007F) == 0x23) ? REL_ENCODING_S : 0) |
			(((instruction & 0x0000007F) == 0x63) ? REL_ENCODING_B : 0) |
			((((instruction & 0x0000007F) == 0x37) || ((instruction & 0x0000007F) == 0x17)) ? REL_ENCODING_U : 0) |
			(((instruction & 0x0000007F) == 0x6F) ? REL_ENCODING_J : 0);
		information_information->opcode = (instruction >> 0) & 0x0000007F;
		information_information->rd = (instruction >> 7) & 0x0000001F;
		information_information->function3 = (instruction >> 12) & 0x00000007;
		information_information->rs1 = (instruction >> 15) & 0x0000001F;
		information_information->rs2 = (instruction >> 20) & 0x0000001F;
		information_information->function7 = (instruction >> 25) & 0x0000007F;
		information_information->compressed_function6 = 0;
		information_information->compressed_function4 = 0;
		information_information->compressed_function3 = 0;

		switch (information_information->encoding)
		{
			case REL_ENCODING_X:
				information_information->intermediate = 0;
				break;
			case REL_ENCODING_R:
				information_information->intermediate = 0;
				break;
			case REL_ENCODING_I:
				information_information->intermediate =
					(((uint32_t)0 - (instruction >> 31)) & ~0x000007FF) |
					(instruction >> 20);
				break;
			case REL_ENCODING_S:
				information_information->intermediate =
					(((uint32_t)0 - (instruction >> 31)) & ~0x000007FF) |
					((instruction >> 20) & 0x000007E0) |
					((instruction >> 7) & 0x0000001F);
				break;
			case REL_ENCODING_B:
				information_information->intermediate =
					(((uint32_t)0 - (instruction >> 31)) & ~0x00000FFF) |
					((instruction << 4) & 0x00000800) |
					((instruction >> 20) & 0x000007E0) |
					((instruction >> 7) & 0x0000001E);
				break;
			case REL_ENCODING_U:
				information_information->intermediate = instruction & 0xFFFFF000;
				break;
			case REL_ENCODING_J:
				information_information->intermediate =
					(((uint32_t)0 - (instruction >> 31)) & ~0x000FFFFF) |
					(instruction & 0x000FF000) |
					((instruction >> 9) & 0x00000800) |
					((instruction >> 20) & 0x000003FE);
				break;
			default:
				break;
		}
	}

	int instruction_index = 0;
	while (instruction_index != (sizeof(instruction_table) / sizeof(*instruction_table)) && (instruction & instruction_table[instruction_index].constant_mask) != instruction_table[instruction_index].constant)
		++instruction_index;

	if (instruction_index != (sizeof(instruction_table) / sizeof(*instruction_table)))
	{
		information_information->mnemonic = instruction_table[instruction_index].mnemonic;
		information_information->module = instruction_table[instruction_index].module;
		information_information->instruction_index = instruction_index;
	}
	else
	{
		information_information->mnemonic = "unknown";
		information_information->module = "unknown";
		information_information->instruction_index = -1;
	}

	information_information->machine_code = instruction;
}

int rel32_get_register_name(int context, int number, int use_abi_name, char** pointer_to_name_pointer, size_t* pointer_name_size)
{
	if (context == REL_REGISTER_CONTEXT_GENERAL)
	{
		static const struct { size_t register_name_size; const char* register_name; size_t abi_name_size; const char* abi_name; } genarel_register_table[32] = {
			{ 2, "x0", 4, "zero" },
			{ 2, "x1", 2, "ra" },
			{ 2, "x2", 2, "sp" },
			{ 2, "x3", 2, "gp" },
			{ 2, "x4", 2, "tp" },
			{ 2, "x5", 2, "t0" },
			{ 2, "x6", 2, "t1" },
			{ 2, "x7", 2, "t2" },
			{ 2, "x8", 2, "s0" },
			{ 2, "x9", 2, "s1" },
			{ 3, "x10", 2, "a0" },
			{ 3, "x11", 2, "a1" },
			{ 3, "x12", 2, "a2" },
			{ 3, "x13", 2, "a3" },
			{ 3, "x14", 2, "a4" },
			{ 3, "x15", 2, "a5" },
			{ 3, "x16", 2, "a6" },
			{ 3, "x17", 2, "a7" },
			{ 3, "x18", 2, "s2" },
			{ 3, "x19", 2, "s3" },
			{ 3, "x20", 2, "s4" },
			{ 3, "x21", 2, "s5" },
			{ 3, "x22", 2, "s6" },
			{ 3, "x23", 2, "s7" },
			{ 3, "x24", 2, "s8" },
			{ 3, "x25", 2, "s9" },
			{ 3, "x26", 3, "s10" },
			{ 3, "x27", 3, "s11" },
			{ 3, "x28", 2, "t3" },
			{ 3, "x29", 2, "t4" },
			{ 3, "x30", 2, "t5" },
			{ 3, "x31", 2, "t6" } };
		if (number < 32)
		{
			if (use_abi_name)
			{
				*pointer_to_name_pointer = (char*)genarel_register_table[number].abi_name;
				*pointer_name_size = genarel_register_table[number].abi_name_size;
			}
			else
			{
				*pointer_to_name_pointer = (char*)genarel_register_table[number].register_name;
				*pointer_name_size = genarel_register_table[number].register_name_size;
			}
			return 0;
		}
		else
			return ENOENT;
	}
	else if (context == REL_REGISTER_CONTEXT_PC)
	{
		if (!number)
		{
			*pointer_to_name_pointer = "pc";
			*pointer_name_size = 2;
			return 0;
		}
		else
			return ENOENT;
	}
	else
		return ENOENT;
}

int rel32_disassemble_instruction(int flags, const void* base_address, uint32_t address_of_instruction, size_t assembly_buffer_size, size_t* assembly_size, char* assembly_buffer)
{
	rel32_instruction_information_t info;
	rel32_decode_instruction((const void*)((uintptr_t)base_address + (uintptr_t)address_of_instruction), &info);

	char* write = assembly_buffer;
	char* write_limit = assembly_buffer + assembly_buffer_size;
	char* register_name;
	size_t part_size;

	if (flags & REL_DISASSEMBLE_ADDRESS)
	{
		if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 9)
			return ENOBUFS;
		part_size = rel32_print_hex(address_of_instruction, write);
		write[part_size] = ' ';
		write += part_size + 1;
	}

	if (flags & REL_DISASSEMBLE_MACHINE_CODE)
	{
		if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 9)
			return ENOBUFS;
		part_size = rel32_print_hex_digits(info.size * 2, info.machine_code, write);
		write[part_size] = ' ';
		write += part_size + 1;
	}

	if (info.instruction_index != -1)
	{
		if (flags & REL_DISASSEMBLE_ENCODING)
		{
			if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 4)
				return ENOBUFS;

			static const char encoding_types[7] = { 'x', 'r', 'i', 's', 'b', 'u', 'j' };
			write[0] = '(';
			write[1] = encoding_types[info.encoding];
			write[2] = ')';
			write[3] = ' ';
			write += 4;
		}

		part_size = rel32_string_size(info.mnemonic);
		if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size)
			return ENOBUFS;
		rel32_copy(write, info.mnemonic, part_size);
		write += part_size;

		switch (instruction_table[info.instruction_index].assembly_encoding)
		{
			case REL_ENCODING_X:
				// unknown encoding
				break;
			case REL_ENCODING_R:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs2, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				break;
			case REL_ENCODING_I:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_S:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs2, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_B:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs2, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_U:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_J:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_I_SHIFT:
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 4)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_unsigned(info.intermediate & 0x1F, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_I_FENCE:
				// this needs more work
				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rd, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 1)
					return ENOBUFS;
				*write = ' ';
				rel32_copy(write + 1, register_name, part_size);
				write += part_size + 1;

				rel32_get_register_name(REL_REGISTER_CONTEXT_GENERAL, info.rs1, flags & REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, &register_name, &part_size);
				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < part_size + 2)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				rel32_copy(write + 2, register_name, part_size);
				write += part_size + 2;

				if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 13)
					return ENOBUFS;
				write[0] = ',';
				write[1] = ' ';
				part_size = rel32_print_signed(*(int32_t*)&info.intermediate, write + 2);
				write += part_size + 2;

				break;
			case REL_ENCODING_I_ENVIROMENT:
				// no thing to be printed here
				break;
			default:
				break;
		}
	}
	else
	{
		if ((size_t)((uintptr_t)write_limit - (uintptr_t)write) < 7)
			return ENOBUFS;
		rel32_copy(write, "unknown", 7);
		write += 7;
	}

	if (flags & REL_DISASSEMBLE_NEW_LINE)
	{
		if (!(size_t)((uintptr_t)write_limit - (uintptr_t)write))
			return ENOBUFS;
		*write = '\n';
		write++;
	}

	*assembly_size = (size_t)((uintptr_t)write - (uintptr_t)assembly_buffer);
	return 0;
}

void rel32i_step_instruction(const void* code_base_address, void* data_base_address, rel32i_register_set_t* register_set)
{
	rel32_instruction_information_t info;
	rel32_decode_instruction((const void*)((uintptr_t)code_base_address + (uintptr_t)register_set->pc), &info);

	uint32_t rs1 = info.rs1 ? register_set->x1_x31[info.rs1 - 1] : 0;
	uint32_t rs2 = info.rs2 ? register_set->x1_x31[info.rs2 - 1] : 0;
	uint32_t effective_address = rs1 + info.intermediate;
	uint32_t rd;
	int set_rd = 0;

	switch (info.instruction_index)
	{
		case 0:/*lui*/
		{
			rd = info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 1:/*auipc*/
		{
			rd = register_set->pc + info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 2:/*jal*/
		{
			rd = register_set->pc + 4;
			set_rd = 1;
			register_set->pc += info.intermediate;
			break;
		}
		case 3:/*jalr*/
		{
			rd = register_set->pc + 4;
			set_rd = 1;
			register_set->pc = (rs1 + info.intermediate) & 0xFFFFFFFE;
			break;
		}
		case 4:/*beq*/
		{
			if (rs1 == rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 5:/*bne*/
		{
			if (rs1 != rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 6:/*blt*/
		{
			if (*(int32_t*)&rs1 < *(int32_t*)&rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 7:/*bge*/
		{
			if (*(int32_t*)&rs1 > *(int32_t*)&rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 8:/*bltu*/
		{
			if (rs1 < rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 9:/*bgeu*/
		{
			if (rs1 > rs2)
				register_set->pc += info.intermediate;
			else
				register_set->pc += 4;
			break;
		}
		case 10:/*lb*/
		{
			rd = (uint32_t)*(uint8_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address);
			rd = ((0 - (rd >> 7)) & 0xFFFFFF00) | rd;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 11:/*lh*/
		{
			rd = (uint32_t)*(uint16_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address);
			rd = ((0 - (rd >> 15)) & 0xFFFF0000) | rd;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 12:/*lw*/
		{
			rd = *(uint32_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 13:/*lbu*/
		{
			rd = (uint32_t)*(uint8_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 14:/*lhu*/
		{
			rd = (uint32_t)*(uint16_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 15:/*sb*/
		{
			*(uint8_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address) = (uint8_t)rs2;
			register_set->pc += 4;
			break;
		}
		case 16:/*sh*/
		{
			*(uint16_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address) = (uint16_t)rs2;
			register_set->pc += 4;
			break;
		}
		case 17:/*sw*/
		{
			*(uint32_t*)((uintptr_t)data_base_address + (uintptr_t)effective_address) = rs2;
			register_set->pc += 4;
			break;
		}
		case 18:/*addi*/
		{
			rd = rs1 + info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 19:/*slti*/
		{
			if (*(int32_t*)&rs1 < *(int32_t*)&info.intermediate)
				rd = 1;
			else
				rd = 0;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 20:/*sltiu*/
		{
			if (rs1 < info.intermediate)
				rd = 1;
			else
				rd = 0;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 21:/*xori*/
		{
			rd = rs1 ^ info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 22:/*ori*/
		{
			rd = rs1 | info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 23:/*andi*/
		{
			rd = rs1 & info.intermediate;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 24:/*slli*/
		{
			rd = rs1 << (info.intermediate & 0x1F);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 25:/*srli*/
		{
			rd = rs1 >> (info.intermediate & 0x1F);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 26:/*srai*/
		{
			uint32_t shift = info.intermediate & 0x1F;
			rd = ((rs1 >> shift) & (0xFFFFFFFF >> shift)) | ((0 - (rs1 >> 31)) & ~(0xFFFFFFFF >> shift));
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 27:/*add*/
		{
			rd = rs1 + rs2;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 28:/*sub*/
		{
			rd = rs1 - rs2;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 29:/*sll*/
		{
			rd = rs1 << (rs2 & 0x1F);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 30:/*slt*/
		{
			if (*(int32_t*)&rs1 < *(int32_t*)&rs2)
				rd = 1;
			else
				rd = 0;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 31:/*sltu*/
		{
			if (rs1 < rs2)
				rd = 1;
			else
				rd = 0;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 32:/*xor*/
		{
			rd = rs1 ^ rs2;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 33:/*srl*/
		{
			rd = rs1 >> (rs2 & 0x1F);
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 34:/*sra*/
		{
			uint32_t shift = rs2 & 0x1F;
			rd = ((rs1 >> shift) & (0xFFFFFFFF >> shift)) | ((0 - (rs1 >> 31)) & ~(0xFFFFFFFF >> shift));
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 35:/*or*/
		{
			rd = rs1 | rs2;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 36:/*and*/
		{
			rd = rs1 & rs2;
			set_rd = 1;
			register_set->pc += 4;
			break;
		}
		case 37:/*fence*/
		{
			register_set->pc += 4;
			break;
		}
		case 38:/*ecall*/
		{
			register_set->pc += 4;
			break;
		}
		case 39:/*ebreak*/
		{
			register_set->pc += 4;
			break;
		}
		default:
		{
			register_set->pc += 4;
			break;
		}
	}

	if (set_rd && info.rd)
		register_set->x1_x31[info.rd - 1] = rd;
}
