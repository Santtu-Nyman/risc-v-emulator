#include "rea_file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static int rea_win32_get_working_directory_path(size_t path_buffer_size, size_t* path_size, char* path_buffer)
{
	static const size_t maximum_length = 0x7FFF;

	size_t path_length = (size_t)GetCurrentDirectoryW(0, 0);

	if (!path_length || path_length > maximum_length)
		return EIO;
	--path_length;

	HANDLE heap = GetProcessHeap();
	if (!heap)
		return ENOMEM;

	WCHAR* wide_buffer = (WCHAR*)HeapAlloc(heap, 0, (path_length + 1) * sizeof(WCHAR));
	if (!wide_buffer)
		return ENOMEM;

	if ((size_t)GetCurrentDirectoryW((DWORD)(path_length + 1), wide_buffer) == path_length)
	{
		HeapFree(heap, 0, wide_buffer);
		return EIO;
	}

	int append_slash = !path_length || (wide_buffer[path_length - 1] != L'\\' && wide_buffer[path_length - 1] != L'/');

	size_t size = (size_t)WideCharToMultiByte(CP_UTF8, 0, wide_buffer, path_length, 0, 0, 0, 0);
	if (!size)
	{
		HeapFree(heap, 0, wide_buffer);
		return EBADMSG;
	}

	*path_size = size + (size_t)append_slash;
	if (size + (size_t)append_slash > path_buffer_size)
	{
		HeapFree(heap, 0, wide_buffer);
		return ENOBUFS;
	}

	if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer, path_length, path_buffer, size, 0, 0) != size)
	{
		HeapFree(heap, 0, wide_buffer);
		return EBADMSG;
	}

	if (append_slash)
		path_buffer[size] = '\\';

	return 0;
}

static int rea_get_program_directory_path(size_t path_buffer_size, size_t* path_size, char* path_buffer)
{
	static const size_t maximum_length = 0x7FFF;

	HANDLE heap = GetProcessHeap();
	if (!heap)
		return ENOMEM;

	WCHAR* wide_buffer = (WCHAR*)HeapAlloc(heap, 0, (maximum_length + 1) * sizeof(WCHAR));
	if (!wide_buffer)
		return ENOMEM;

	HMODULE executable_module = GetModuleHandleW(0);
	if (!executable_module)
	{
		HeapFree(heap, 0, wide_buffer);
		return EIO;
	}

	size_t path_length = (size_t)GetModuleFileNameW(executable_module, wide_buffer, maximum_length + 1);
	if (!path_length || path_length > maximum_length)
	{
		HeapFree(heap, 0, wide_buffer);
		return EIO;
	}

	while (path_length && wide_buffer[path_length - 1] != L'\\' && wide_buffer[path_length - 1] != L'/')
		--path_length;

	size_t size = (size_t)WideCharToMultiByte(CP_UTF8, 0, wide_buffer, path_length, 0, 0, 0, 0);
	if (!size)
	{
		HeapFree(heap, 0, wide_buffer);
		return EBADMSG;
	}

	*path_size = size;
	if (size > path_buffer_size)
	{
		HeapFree(heap, 0, wide_buffer);
		return ENOBUFS;
	}

	if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer, path_length, path_buffer, size, 0, 0) != size)
	{
		HeapFree(heap, 0, wide_buffer);
		return EBADMSG;
	}

	HeapFree(heap, 0, wide_buffer);
	return 0;
}

int rea_get_special_directory_path(int special_directory, size_t path_buffer_size, size_t* path_size, char* path_buffer)
{
	switch (special_directory)
	{
		case REA_IGNORE_DIRECTORY :
			*path_size = 0;
			return 0;
		case REA_WORKING_DIRECTORY :
			return rea_win32_get_working_directory_path(path_buffer_size, path_size, path_buffer);
		case REA_PROGRAM_DIRECTORY :
			return rea_get_program_directory_path(path_buffer_size, path_size, path_buffer);
		default:
			return ENOENT;
	}
}

#endif // _WIN32

int rea_load_file(int special_directory, const char* file_name, size_t file_data_buffer_size, size_t* file_size, void* file_data_buffer)
{
	size_t directore_path_size;
	int error = rea_get_special_directory_path(special_directory, 0, &directore_path_size, 0);
	if (error && error != ENOBUFS)
		return error;

	size_t end_path_size = strlen(file_name);

	char* name = (char*)malloc(directore_path_size + end_path_size + 1);
	if (!name)
		return ENOMEM;

	size_t final_directore_path_size;
	error = rea_get_special_directory_path(special_directory, directore_path_size, &final_directore_path_size, name);
	if (!error && final_directore_path_size != directore_path_size)
		error = EIO;
	if (error)
	{
		free(name);
		return error;
	}
	memcpy(name + directore_path_size, file_name, end_path_size + 1);

	FILE* handle = fopen(name, "rb");
	if (!handle)
		error = errno;
	free(name);
	if (error)
		return error;

	if (fseek(handle, 0, SEEK_END))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	size_t size = (size_t)ftell(handle);
	if (size == (size_t)-1)
	{
		error = errno;
		fclose(handle);
		return error;
	}

	*file_size = size;
	if (size > file_data_buffer_size)
	{
		error = ENOBUFS;
		fclose(handle);
		return error;
	}

	if (fseek(handle, 0, SEEK_SET))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}

	for (size_t index = 0; index != size;)
	{
		size_t read = fread((void*)((uintptr_t)file_data_buffer + index), 1, size - index, handle);
		if (!read)
		{
			error = ferror(handle);
			if (!error)
				error = EIO;
			fclose(handle);
			return error;
		}
		index += read;
	}

	fclose(handle);
	return 0;
}

int rea_store_file(int special_directory, const char* file_name, size_t file_size, void* file_data_buffer)
{
	return ENOSYS;
}

int rea32_load_binary_file(int special_directory, const char* file_name, rel32_binary_t** pointer_to_binary)
{
	const size_t max_line_size = 128;
	const size_t header_size = ((sizeof(rel32_binary_t) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1));
	size_t file_name_length = strlen(file_name);
	size_t file_name_size = (file_name_length & (sizeof(void*) - 1)) ? (((file_name_length + 1) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1)) : (file_name_length + sizeof(void*));
	size_t file_size;
	int error = rea_load_file(special_directory, file_name, 0, &file_size, 0);
	rel32_binary_t* binary = (rel32_binary_t*)malloc(header_size + file_name_size + file_size);
	if (!binary)
		return ENOMEM;
	size_t binary_size;
	memcpy((void*)((uintptr_t)binary + header_size), file_name, file_name_length + 1);
	error = rea_load_file(special_directory, file_name, file_size, &binary_size, (void*)((uintptr_t)binary + header_size + file_name_size));
	if (error)
	{
		free(binary);
		return EIO;
	}
	else if (binary_size != file_size)
	{
		free(binary);
		return EBUSY;
	}
	size_t instruction_count = binary_size / sizeof(uint32_t);
	size_t disassembly_buffer_size = instruction_count * max_line_size;
	size_t disassembly_length = 0;
	char* disassembly_buffer = (char*)malloc(disassembly_buffer_size);
	if (!disassembly_buffer)
	{
		free(binary);
		return ENOMEM;
	}
	for (size_t pc = 0; pc != instruction_count * 4; pc += 4)
	{
		if (disassembly_buffer_size - disassembly_length < max_line_size)
		{
			disassembly_buffer_size += max_line_size;
			char* new_disassembly_buffer = (char*)realloc(disassembly_buffer, disassembly_buffer_size);
			if (!new_disassembly_buffer)
			{
				free(disassembly_buffer);
				free(binary);
				return ENOMEM;
			}
			disassembly_buffer = new_disassembly_buffer;
		}
		size_t line_size = 0;
		error = rel32_disassemble_instruction(
			REL_DISASSEMBLE_MACHINE_CODE | REL_DISASSEMBLE_NEW_LINE | REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS,
			(void*)((uintptr_t)binary + header_size + file_name_size),
			(uint32_t)pc, disassembly_buffer_size - disassembly_length,
			&line_size, disassembly_buffer + disassembly_length);
		if (error)
		{
			free(disassembly_buffer);
			free(binary);
			return error;
		}
		disassembly_length += line_size;
	}
	size_t disassembly_size = (disassembly_length & (sizeof(void*) - 1)) ? (((disassembly_length + 1) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1)) : (disassembly_length + sizeof(void*));
	rel32_binary_t* new_binary = (rel32_binary_t*)realloc(binary, header_size + file_name_size + file_size + disassembly_size);
	if (!new_binary)
	{
		free(disassembly_buffer);
		free(binary);
		return ENOMEM;
	}
	binary = new_binary;
	memcpy((void*)((uintptr_t)binary + header_size + file_name_size + binary_size), disassembly_buffer, disassembly_length);
	*(char*)((uintptr_t)binary + header_size + file_name_size + binary_size + disassembly_length) = 0;
	free(disassembly_buffer);
	binary->file_name = (char*)((uintptr_t)binary + header_size);
	binary->size = binary_size;
	binary->instruction_count = instruction_count;
	binary->data = (void*)((uintptr_t)binary + header_size + file_name_size);
	binary->disassembly_length = disassembly_length;
	binary->disassembly = (char*)((uintptr_t)binary + header_size + file_name_size + binary_size);
	*pointer_to_binary = binary;
	return 0;
}