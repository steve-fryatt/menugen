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


/* Local source headers. */

#include "parse.h"


struct file_head_block {
	int		dialogues;
	int		indirection;
	int		validation;
};

struct file_menu_head_block {
	int		zero;
	int		flags;
};

struct file_menu_start_block {
	int		next;
	int		submenus;
};


/**
 * Load a file into memory, returning a pointer to a malloc()-claimed block
 * and optionally the size of the data.
 *
 * \param *file		Pointer to the file data to be processed.
 * \param length	The length of the data block.
 */

void parse_process(int8_t *file, size_t length)
{
	int				menu_offset;
	struct file_menu_start_block	*menu;

	if (file == NULL)
		return;

	menu_offset = 12;

	printf("File: %d\n", file);

	while (menu_offset != -1) {
		printf("Menu at offset %d\n", menu_offset);
		menu = (struct file_menu_start_block *) (file + menu_offset);
	
		printf("Menu: %d\n", menu);
	
		printf("Next offset %d\n", menu->next);
	
		menu_offset = menu->next;
	}
}

