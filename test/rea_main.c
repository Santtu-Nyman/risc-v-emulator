#include "rel_risc_v_emulator.h"
#include "rea_file.h"
#include "rea_gui.h"
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

int rea_create_emulator_gui(rea_gui_t** gui);

int main(int argc, char** argv)
{
	rel32i_register_set_t register_set = { 0 };
	rel32_binary_t* binary = 0;
	rea_gui_t* gui;
	int create_error = rea_create_emulator_gui(&gui);

	uint32_t frame_count = 0;
	for (int run = 1; run; ++frame_count)
	{
		while (rea_poll_event(gui))
		{
			switch (gui->event.type)
			{
				case SDL_MOUSEBUTTONDOWN:
				{
					if (gui->selected_window)
					{
						if (gui->selected_window->id == REA_EXECUTE_BOX_WINDOW_ID)
						{
							rel32i_step_instruction(binary->data, 0, &register_set);
						}
						else if (gui->selected_window->id == REA_LOAD_BIN_WINDOW_ID)
						{
							rea_window_t* file_window = rea_get_window_by_id(gui, REA_TOP_FILE_BAR_TEXT_BOX_WINDOW_ID);
							if (file_window && file_window->text)
							{
								rel32_binary_t* new_binary;
								if (!rea32_load_binary_file(REA_IGNORE_DIRECTORY, file_window->text, &new_binary))
								{
									if (binary)
										free(binary);
									binary = new_binary;
									rea_set_window_text(gui, REA_CODE_SEQUENCE_VALUE_WINDOW_ID, binary->disassembly);
								}
							}
						}
						else if (gui->selected_window->id == REA_RESET_BOX_WINDOW_ID)
						{
							memset(&register_set, 0, sizeof(rel32i_register_set_t));
						}
					}
					break;
				}
				case SDL_WINDOWEVENT:
				{
					//if (gui->event.window.event == SDL_WINDOWEVENT_RESIZED)
					//	rea_scale_emulator_gui_windows(gui, gui->event.window.data1, gui->event.window.data2);
					break;
				}
				case SDL_QUIT:
				{
					run = 0;
					break;
				}
				default:
					break;
			}
		}

		switch (rea_get_window_by_id(gui, REA_REGISTER_FORMAT_MENU_WINDOW_ID)->paint_data.selected_item)
		{
			case 0:/*hex*/
			{
				rea_set_window_text_with_hexadecimal_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 0, register_set.pc);
				for (size_t i = 0; i != 31; ++i)
					rea_set_window_text_with_hexadecimal_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 1 + i, register_set.x1_x31[i]);
				break;
			}
			case 1:/*unsigned*/
			{
				rea_set_window_text_with_unsigned_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 0, register_set.pc);
				for (size_t i = 0; i != 31; ++i)
					rea_set_window_text_with_unsigned_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 1 + i, register_set.x1_x31[i]);
				break;
			}
			case 2:/*signed*/
			{
				rea_set_window_text_with_signed_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 0, *(int32_t*)&register_set.pc);
				for (size_t i = 0; i != 31; ++i)
					rea_set_window_text_with_signed_number(gui, REA_REGISTER_VALUE_WINDOW_ID + 1 + i, *(int32_t*)&register_set.x1_x31[i]);
				break;
			}
			default:
			{
				rea_set_window_text(gui, REA_REGISTER_VALUE_WINDOW_ID + 0, "-");
				for (size_t i = 0; i != 31; ++i)
					rea_set_window_text(gui, REA_REGISTER_VALUE_WINDOW_ID + 1 + i, "-");
				break;
			}
		}

		rea_set_window_text(gui, REA_FRAME_COUNT_WINDOW_ID, "Frame count: ");
		rea_append_window_text_with_unsigned_number(gui, REA_FRAME_COUNT_WINDOW_ID, frame_count);

		rea_draw_gui(gui);

		uint32_t current_time = SDL_GetTicks();
		if (current_time < (gui->frame_timestamp + gui->frame_duration))
			SDL_Delay((gui->frame_timestamp + gui->frame_duration) - current_time);
		gui->frame_timestamp += gui->frame_duration;
	}

	if (binary)
		free(binary);





	



	/*
	if (!strcmp(test_text_buffer, "test"))
	{
		SDL_Surface* screen_shot = SDL_CreateRGBSurface(0, gui->window_table[0].w, gui->window_table[0].h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
		SDL_RenderReadPixels(gui->renderer_handle, NULL, SDL_PIXELFORMAT_RGBA8888, screen_shot->pixels, screen_shot->pitch);
		SDL_SaveBMP(screen_shot, "screenshot.bmp");
		SDL_FreeSurface(screen_shot);

		assert(0);
	}
	*/

	/*

	for (uint32_t pc = 0; pc < (uint32_t)file_size; pc += 4)
	{
		char asm_buffer[128];
		size_t asm_size = 0;
		rel32_disassemble_instruction(REL_DISASSEMBLE_ENCODING | REL_DISASSEMBLE_MACHINE_CODE | REL_DISASSEMBLE_NEW_LINE | REL_DISASSEMBLE_ABI_REGISTER_MNEMONICS, file_data, pc, 127, &asm_size, asm_buffer);
		asm_buffer[asm_size] = 0;
		printf("%s", asm_buffer);
	}

	printf("\n");

	char* disassemblyt_buffer = malloc(0x10000);

	for (uint32_t pc = 0; pc < (uint32_t)file_size; pc += 4)
	{
		char asm_buffer[128];
		disasm_inst(asm_buffer, 128, rv32, 0, *(uint32_t*)((uintptr_t)file_data + (uintptr_t)pc));

		printf("%s\n", asm_buffer);
	}

	getchar();
	*/

	return 0;

}

int rea_create_emulator_gui(rea_gui_t** gui)
{
	/* RRGGBBAA */
	static const struct { size_t register_name_size; const char* register_name; size_t abi_name_size; const char* abi_name; } register_window_name_table[32] = {
			{ 2, "pc", 2, "pc" },
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

	const uint32_t gui_background_color = 0x202020FF;
	const uint32_t gui_border_color = 0xC0C0C0FF;
	const uint32_t gui_text_color = 0xFFFFFFFF;
	const uint32_t gui_text_size = 18;

	rea_window_t window_table[16 + 64] = {
		{ 0, 0, 800, 600, 0, gui_text_size, 0, 0, REA_MAIN_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 0, 0 }, /* main window */
		{ 640, 576, 160, 24, 0, gui_text_size, REA_WINDOW_IS_TOP_LEVEL, REA_MAIN_WINDOW_ID, REA_FRAME_COUNT_WINDOW_ID, gui_background_color, gui_border_color, 0xFFFF00FF, 13, "Frame count 0" },
		{ 0, 0, 800, 54, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, 0, REA_MAIN_WINDOW_ID, REA_TOP_BAR_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 0, 0 }, /* top tool bar */
		{ 0, 30, 800, 24, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, 0, REA_TOP_BAR_WINDOW_ID, REA_TOP_FILE_BAR_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 12, "Binary file:" }, /* top text box */
		{ 84, 0, 716, 24, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_WINDOW_IS_SELECTABLE, REA_TOP_FILE_BAR_WINDOW_ID, REA_TOP_FILE_BAR_TEXT_BOX_WINDOW_ID, 0x000000FF, gui_border_color, gui_text_color, 63, "C:\\Users\\Illuminati\\Desktop\\RV32I-Simulator-master\\tests\\t5.bin" },
		{ 0, 0, 128, 32, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_WINDOW_IS_SELECTABLE | REA_CENTER_WINDOW_TEXT_HORIZONTALLY | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_TOP_BAR_WINDOW_ID, REA_LOAD_BIN_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 11, "Load binary" },
		{ 126, 0, 128, 32, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_WINDOW_IS_SELECTABLE | REA_CENTER_WINDOW_TEXT_HORIZONTALLY | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_TOP_BAR_WINDOW_ID, REA_RESET_BOX_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 14, "Reset emulator" },
		{ 252, 0, 128, 32, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_WINDOW_IS_SELECTABLE | REA_CENTER_WINDOW_TEXT_HORIZONTALLY | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_TOP_BAR_WINDOW_ID, REA_EXECUTE_BOX_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 7, "Execute" },
		{ 378, 0, 288, 32, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_TOP_BAR_WINDOW_ID, REA_REGISTER_FORMAT_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 24, "Register display format:" },
		{ 174, 4, 110, 24, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, REA_WINDOW_IS_SELECTABLE | REA_WINDOW_IS_DROPDOWN_MENU | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_REGISTER_FORMAT_WINDOW_ID, REA_REGISTER_FORMAT_MENU_WINDOW_ID, 0x000000FF, gui_border_color, gui_text_color, 19, "hex\nunsigned\nsigned" },
		{ 0, 52, 256, 548, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, 0, REA_MAIN_WINDOW_ID, REA_CODE_SEQUENCE_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 0, 0 }, /* code sequence */
		{ 2 * REA_DEFAULT_BORDER_THICKNESS, 2 * REA_DEFAULT_BORDER_THICKNESS, 256 - (4 * REA_DEFAULT_BORDER_THICKNESS), 24, 0, gui_text_size, REA_CENTER_WINDOW_TEXT_HORIZONTALLY | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_CODE_SEQUENCE_WINDOW_ID, REA_CODE_SEQUENCE_TITLE_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 4, "Code" }, /* code sequence title */
		{ 2, 32, 252, 514, 0, gui_text_size, 0, REA_CODE_SEQUENCE_WINDOW_ID, REA_CODE_SEQUENCE_VALUE_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 0, 0 }, /* code sequence */
		{ 254, 52, 336, 548, REA_DEFAULT_BORDER_THICKNESS, gui_text_size, 0, REA_MAIN_WINDOW_ID, REA_REGISTER_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 0, 0 }, /* register window */
		{ 2 * REA_DEFAULT_BORDER_THICKNESS, 2 * REA_DEFAULT_BORDER_THICKNESS, 336 - (4 * REA_DEFAULT_BORDER_THICKNESS), 24, 0, gui_text_size, REA_CENTER_WINDOW_TEXT_HORIZONTALLY | REA_CENTER_WINDOW_TEXT_VERTICALLY, REA_REGISTER_WINDOW_ID, REA_REGISTER_TITLE_WINDOW_ID, gui_background_color, gui_border_color, gui_text_color, 9, "Registers" },
		{ 590, 52, 210, 548, 0, gui_text_size, REA_CENTER_WINDOW_TEXT_HORIZONTALLY, REA_MAIN_WINDOW_ID, REA_MEMORY_MAP_WINDOW_ID, gui_background_color, gui_border_color, 0xFFFF00FF, 22, "Memory map placeholder" } /* memory map window */
	};

	

	for (size_t i = 0; i != 32; ++i)
	{
		rea_window_t* register_frame_window = window_table + 16 + (2 * i) + 0;
		memset(register_frame_window, 0, sizeof(rea_window_t));
		register_frame_window->x = (i < 16) ? 16 : (16 + 144 + 16);
		register_frame_window->y = (32 + (i * 32) - ((i < 16) ? 0 : 16 * 32));
		register_frame_window->w = 144;
		register_frame_window->h = 24;
		register_frame_window->border_thickness = 0;
		register_frame_window->font_size = gui_text_size;
		register_frame_window->style_flags = REA_CENTER_WINDOW_TEXT_VERTICALLY;
		register_frame_window->parent_id = REA_REGISTER_WINDOW_ID;
		register_frame_window->id = REA_REGISTER_FRAME_WINDOW_ID + i;
		register_frame_window->background_color = gui_background_color;
		register_frame_window->border_color = gui_border_color;
		register_frame_window->text_color = gui_text_color;
		register_frame_window->text_length = register_window_name_table[i].abi_name_size;
		register_frame_window->text = (char*)register_window_name_table[i].abi_name;

		rea_window_t* register_value_window = window_table + 16 + (2 * i) + 1;
		memset(register_value_window, 0, sizeof(rea_window_t));
		register_value_window->x = 36;
		register_value_window->y = 0;
		register_value_window->w = 144 - 36;
		register_value_window->h = 24;
		register_value_window->border_thickness = REA_DEFAULT_BORDER_THICKNESS;
		register_value_window->font_size = gui_text_size;
		register_value_window->style_flags = 0;
		register_value_window->parent_id = register_frame_window->id;
		register_value_window->id = REA_REGISTER_VALUE_WINDOW_ID + i;
		register_value_window->background_color = 0x000000FF;
		register_value_window->border_color = gui_border_color;
		register_value_window->text_color = gui_text_color;
		register_value_window->text_length = 11;
		register_value_window->text = "";
	}

	int error = rea_create_gui(gui, sizeof(window_table) / sizeof(*window_table), window_table, 30);

	return error;
}