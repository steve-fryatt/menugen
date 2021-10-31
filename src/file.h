/* Copyright 1996-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of MenuGen:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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

#define FILE_ITEM_TEXT_LENGTH 12

/**
 * Standard file data structures, used to build the standard menu file.
 */

/**
 * File head block, with offsets to internal data structures.
 */

struct file_head_block {
	int				dialogues;			/**< Offset to the dialogue list.		*/
	int				indirection;			/**< Offset to the indirection data list.	*/
	int				validation;			/**< Offset to the validation data list.	*/
};

/**
 * Extended file head block, which follows the file head block.
 * The zero word at the start allows the format to be differentiated
 * from older versions, as these will always have a non-zero
 * 'next menu' offset here.
 */

struct file_extended_head_block {
	int				zero;				/**< Zero to signify new data format.		*/
	int				flags;				/**< File format flags (unused at present).	*/
	int				menus;				/**< Offset to the menu list.			*/
	int				end;				/**< Zero to indicate the end of the header.	*/
};

/**
 * The block of icon data associated with an indirected text icon in a Wimp
 * icon block.
 */

struct file_indirected_text {
	int				indirection;			/**< Pointer to the indirected text.		*/
	int				validation;			/**< Pointer to the validation string.		*/
	int				size;				/**< Size of the indirected text, in bytes.	*/
};

/**
 * Menu item or title text, which can either be a straight text string
 * or an indirected text block.
 */

union file_item_text {
	char				text[FILE_ITEM_TEXT_LENGTH];	/**< A straight text string.			*/
	struct file_indirected_text	indirected_text;		/**< An indirected text data block.		*/
};

/**
 * Wimp menu header block.
 */

struct file_menu_block {
	int				next;
	int				submenus;
	union file_item_text		title_data;			/**< The menu title text.			*/
	wimp_colour			title_fg;			/**< The menu title foreground colour.		*/
	wimp_colour			title_bg;			/**< The menu title background colour.		*/
	wimp_colour			work_fg;			/**< The menu workarea foreground colour.	*/
	wimp_colour			work_bg;			/**< The menu workarea background colour.	*/
	int				width;				/**< The width of the menu, in OS units.	*/
	int				height;				/**< The height of a menu entry, in OS units.	*/
	int				gap;				/**< The inter-entry gap, in OS units.		*/
};

/**
 * Wimp menu item block.
 */

struct file_item_block {
	wimp_menu_flags			menu_flags;			/**< The item's menu flags.			*/
	int				submenu_file_offset;		/**< Submenu offset data.			*/
	wimp_icon_flags			icon_flags;			/**< The item's icon flags.			*/
	union file_item_text		icon_data;			/**< The menu item's text.			*/
};

/**
 * Indirection data item.
 */

struct file_indirection_block {
	int				location;			/**< Offset to the associated indirection item.	*/
	char				data[];				/**< Zero length placeholder for the text.	*/
};

/**
 * Validation data item.
 */

struct file_validation_block {
	int				location;			/**< Offset to the associated validation item.	*/
	int				length;				/**< The total length of the block.		*/
	char				data[];				/**< Zero length placeholder for the string.	*/
};

/**
 * New format dialogue header item. This appears at the dialogue data
 * offset in new format files to indicate the new format.
 */

struct file_dialogue_head_block {
	int				zero;				/**< Zero to signify new data format.		*/
};

/**
 * New format dialogue list item.
 */

struct file_dialogue_tag_block {
	int				dialogues;			/**< Offset to the first dialogue entry.	*/
	char				tag[];				/**< Zero length placeholder for the name.	*/
};

/**
 * New format menu list item.
 */

struct file_menu_tag_block {
	int				menu;				/**< Offset to the menu block entry.		*/
	char				tag[];				/**< Zero length placeholder for the name.	*/
};

