/* Copyright 1996-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of MenuGen:
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

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

//#define NULL_OFFSET -1
//#define NO_SUBMENU  -1


/**
 * Standard file data structures, used to build the standard menu file.
 */

/**
 * File head block, with offsets to internal data structures.
 */

struct file_head_block {
	int		dialogues;		/**< Offset to the dialogue list.		*/
	int		indirection;		/**< Offset to the indirection data list.	*/
	int		validation;		/**< Offset to the validation data list.	*/
};

struct file_menu_head_block {
	int		zero;
	int		flags;
};

struct file_menu_start_block {
	int		next;
	int		submenus;
};

struct file_menu_start_name_block{
	int		next;
	int		submenus;
	char		tag[];		/* Placeholder! */
};

/**
 * The block of icon data associated with an indirected text icon in a Wimp
 * icon block.
 */

struct file_indirected_text {
	int		indirection;		/**< Pointer to the indirected text.		*/
	int		validation;		/**< Pointer to the validation string.		*/
	int		size;			/**< Size of the indirected text, in bytes.	*/
};



struct file_menu_block {
	union {
		char				text[12];
		struct file_indirected_text	indirected_text;
	} title_data;
	wimp_colour	title_fg;
	wimp_colour	title_bg;
	wimp_colour	work_fg;
	wimp_colour	work_bg;
	int		width;
	int		height;
	int		gap;
};

struct file_item_block {
	wimp_menu_flags	menu_flags;
	int		submenu_file_offset;
	wimp_icon_flags	icon_flags;
	union {
		char		text[12];
		struct {
			int		indirection;
			int		validation;
			int		size;
		} indirected_text;
	} icon_data;
};

/**
 * Indirection data item.
 */

struct file_indirection_block {
	int		location;		/**< Offset to the associated indirection item.	*/
	char		data[];			/**< Zero length placeholder for the text.	*/
};

/**
 * Validation data item.
 */

struct file_validation_block {
	int		location;		/**< Offset to the associated validation item.	*/
	int		length;			/**< The total length of the block.		*/
	char		data[];			/**< Zero length placeholder for the string.	*/
};

/**
 * New format dialogue header item. This appears at the dialogue data
 * offset in new format files to indicate the new format.
 */

struct file_dialogue_head_block {
	int		zero;			/**< Zero to signify new data format.		*/
};

/**
 * New format dialogue list header.
 */

struct file_dialogue_tag_block {
	int		dialogues;		/**< Offset to the first dialogue entry.	*/
	char		tag[];			/**< Zero length placeholder for the name.	*/
};

