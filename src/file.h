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

struct file_menu_start_name_block{
	int		next;
	int		submenus;
	char		tag[];		/* Placeholder! */
};

struct file_menu_block {
	union {
		char		text[12];
		struct {
			int		indirection;
			int		validation;
			int		size;
		} indirected_text;
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

struct file_indirection_block {
	int		location;
	char		data[];		/* Placeholder! */
};

struct file_validation_block {
	int		location;
	int		length;
	char		data[];		/* Placeholder! */
};

struct file_dialogue_head_block {
	int		zero;
};

struct file_dialogue_tag_block {
	int		dialogues;
	char		tag[];		/* Placeholder! */
};

