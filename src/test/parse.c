/* Copyright 2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of MenuTest:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "parse.h"

#include "../file.h"




static void	parse_process_menus(int8_t *file, size_t length);

/**
 * Load a file into memory, returning a pointer to a malloc()-claimed block
 * and optionally the size of the data.
 *
 * \param *file		Pointer to the file data to be processed.
 * \param length	The length of the data block.
 */

void parse_process(int8_t *file, size_t length)
{
	if (file == NULL)
		return;

	parse_process_menus(file, length);
}



static void parse_process_menus(int8_t *file, size_t length)
{
	int				menu_offset;
	struct file_menu_start_block	*menu_header;
	struct file_menu_block		*menu_block;
	struct file_item_block		*item_block;

	menu_offset = 20;

	while (menu_offset != -1) {
		menu_header = (struct file_menu_start_block *) (file + menu_offset - 8);
		menu_block = (struct file_menu_block *) (file + menu_offset);
		item_block = (struct file_item_block *) (file + menu_offset + 28);


		if ((item_block->menu_flags & wimp_MENU_TITLE_INDIRECTED) != 0) {
			printf("Menu: Indirected Title at %d\n", menu_block->title_data.indirected_text.indirection);
		} else {
			printf("Menu: %s\n", menu_block->title_data.text);
		}

		menu_offset = menu_header->next;
	}

}

