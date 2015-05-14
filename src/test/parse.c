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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "parse.h"

#include "../file.h"



static void	parse_process_indirected_data(int8_t *file, size_t length);
static void	parse_process_validation_data(int8_t *file, size_t length);
static void	parse_process_dialogues(int8_t *file, size_t length);
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

	parse_process_indirected_data(file, length);
	parse_process_validation_data(file, length);
	parse_process_dialogues(file, length);
	parse_process_menus(file, length);
}


/**
 * Process the indirected data in the file, parsing the list of strings and
 * linking in the pointers in the menu data.
 *
 * \param *file		Pointer to the file data to be processed.
 * \param length	The length of the data block.
 */

static void parse_process_indirected_data(int8_t *file, size_t length)
{
	int				offset;
	struct file_head_block		*file_head = (struct file_head_block *) file;
	struct file_indirection_block	*data;
	struct file_indirected_text	*indirection;

	if (file == NULL)
		return;

	printf("\nIndirected Text Data\n");
	printf("--------------------\n");

	if (file_head->indirection == -1) {
		printf("  No Data\n");
		return;
	}

	offset = file_head->indirection;

	do {
		data = (struct file_indirection_block *) (file + offset);

		if (data->location == -1)
			continue;

		indirection = (struct file_indirected_text *) (file + data->location);


		printf("  %d bytes: '%s'\n", indirection->size, data->data);

		/* Store an offset for the indirected text, not a pointer,
		 * as MenuTest could be running on a 64 bit system.
		 */

		indirection->indirection = offset + 4;

		offset += (indirection->size + 7) & (~3);

	} while (data->location != -1);
}


/**
 * Process the validation string data in the file, parsing the list of strings
 * and linking in the pointers in the menu data.
 *
 * \param *file		Pointer to the file data to be processed.
 * \param length	The length of the data block.
 */

static void parse_process_validation_data(int8_t *file, size_t length)
{
	int				offset;
	struct file_head_block		*file_head = (struct file_head_block *) file;
	struct file_validation_block	*data;
	struct file_indirected_text	*indirection;

	if (file == NULL)
		return;

	printf("\nValidation String Data\n");
	printf("----------------------\n");

	if (file_head->validation == -1) {
		printf("  No Data\n");
		return;
	}

	offset = file_head->validation;

	do {
		data = (struct file_validation_block *) (file + offset);

		if (data->location == -1)
			continue;

		indirection = (struct file_indirected_text *) (file + data->location - 4);


		printf("  %d bytes: '%s'\n", data->length, data->data);

		/* Store an offset for the validation string, not a pointer,
		 * as MenuTest could be running on a 64 bit system.
		 */

		indirection->validation = offset + 8;

		offset += data->length;

	} while (data->location != -1);
}


static void parse_process_dialogues(int8_t *file, size_t length)
{
	int				offset;
	struct file_head_block		*file_head = (struct file_head_block *) file;
	struct file_dialogue_head_block	*head;
	struct file_dialogue_tag_block	*data;

	if (file == NULL)
		return;

	printf("\nDialogue Data\n");
	printf("------------\n");

	if (file_head->dialogues == -1) {
		printf("  No Data\n");
		return;
	}

	offset = file_head->dialogues;

	head = (struct file_dialogue_head_block *) (file + offset);

	if (head->zero != 0) {
		printf("  Single dialogue chain (old format)\n");
		return;
	}

	offset += 4;

	do {
		data = (struct file_dialogue_tag_block *) (file + offset);

		if (data->dialogues == -1)
			continue;

		printf("  Dialogue list: '%s'\n", data->tag);

		offset += (strlen(data->tag) + 8) & (~3);

	} while (data->dialogues != -1);
}



static void parse_process_menus(int8_t *file, size_t length)
{
	int				offset;
	struct file_menu_block		*menu_block;
	struct file_item_block		*item_block;
	char				text[FILE_ITEM_TEXT_LENGTH + 1];
	bool				last_item;

	if (file == NULL)
		return;

	text[FILE_ITEM_TEXT_LENGTH] = '\0';

	offset = 20;

	while (offset != -1) {
		menu_block = (struct file_menu_block *) (file + offset - 8);
		item_block = (struct file_item_block *) (file + offset + 28);

		printf("\nMenu Data\n");
		printf("---------\n");

		if (item_block->menu_flags & wimp_MENU_TITLE_INDIRECTED) {
			/* The indirection field is an offset into the file block here,
			 * as MenuTest could be running on a 64 bit system making it
			 * unsafe to store an actual pointer.
			 */
			printf("  Indirected title: '%s'\n", (char *) (file + menu_block->title_data.indirected_text.indirection));
		} else {
			strncpy(text, menu_block->title_data.text, FILE_ITEM_TEXT_LENGTH);
			printf("  Fixed title: '%s'\n", text);
		}

		printf("  Width: %d\n", menu_block->width);
		printf("  Height: %d\n", menu_block->height);
		printf("  Gap: %d\n", menu_block->gap);

		last_item = false;

		do {
			if (item_block->icon_flags & wimp_ICON_INDIRECTED) {
				/* The indirection field is an offset into the file block here,
				 * as MenuTest could be running on a 64 bit system making it
				 * unsafe to store an actual pointer.
				 */
				printf("  * Indirected entry: '%s'\n", (char *) (file + item_block->icon_data.indirected_text.indirection));
			} else {
				strncpy(text, item_block->icon_data.text, FILE_ITEM_TEXT_LENGTH);
				printf("  * Fixed entry: '%s'\n", text);
			}
		
			if (item_block->menu_flags & wimp_MENU_LAST)
				last_item = true;
			else
				item_block += 1;
		} while (!last_item);

		offset = menu_block->next;
	}

}

