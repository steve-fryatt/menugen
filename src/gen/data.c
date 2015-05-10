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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "data.h"

#include "../file.h"

#define NULL_OFFSET -1
#define NO_SUBMENU  -1

/**
 * Internal data structures, used to collect the information together
 * prior to building the menu defs file.
 */

struct item_definition {
	char			*text;
	int			text_len; /* 0 for non-indirected. */
	char			*validation;

	wimp_menu_flags		menu_flags;
	wimp_icon_flags		icon_flags;

	int			icon_foreground;
	int			icon_background;

	char			submenu_tag[MAX_TAG_LEN];
	bool			submenu_dbox; /* True if the item is a dbox. */

	struct menu_definition	*submenu;
	struct dbox_chain_data	*dbox;

	int			next_submenu;

	int			file_offset; /* Where this menu item resides. */

	struct item_definition	*next;
};

struct menu_definition {
	char			tag[MAX_TAG_LEN];
	char			*title;
	int			title_len; /* 0 for non-indirected. */

	bool			reversed;

	int			item_width;
	int			item_height;
	int			item_gap;

	int			title_foreground;
	int			title_background;
	int			work_area_foreground;
	int			work_area_background;

	int			items;
	struct item_definition	*first_item;

	int			first_submenu;

	int			file_offset; /* Where this menu resides. */

	struct menu_definition	*next;
};

struct indirection_data {
	struct menu_definition	*menu;
	struct item_definition	*item;

	int			file_offset;
	int			block_length;
	int			target;

	struct indirection_data	*next;
};

struct validation_data {
	struct item_definition	*item;

	int			string_len;

	int			file_offset;
	int			block_length;
	int			target;

	struct validation_data	*next;
};

struct submenu_data {
	struct item_definition	*item;

	struct submenu_data	*next;
};

struct dbox_data {
	struct item_definition	*item;

	struct dbox_data	*next;
};

struct dbox_chain_data {
	char			*tag;

	int			first_dbox;

	int			file_offset;
	int			block_length;

	struct dbox_chain_data	*next;
};


static struct menu_definition	*menu_list = NULL;
static struct indirection_data	*indirection_list = NULL;
static struct validation_data	*validation_list = NULL;
static struct submenu_data	*submenu_list = NULL;
static struct dbox_data		*dbox_list = NULL;
static struct dbox_chain_data	*dbox_chain_list = NULL;

static struct menu_definition	*current_menu = NULL;
static struct item_definition	*current_item = NULL;

static int			dbox_offset = NULL_OFFSET;

static int			longest_menu_start = 0;
static int			longest_indirection = 0;
static int			longest_validation = 0;
static int			longest_dbox_chain = 0;

struct menu_definition		*data_find_menu_from_tag(char *tag);
struct dbox_chain_data		*data_find_dbox_chain_from_tag(char *tag);
static char			*data_boolean_yes_no(int value);

/**
 * Go through the assembled menu structures, filling in the missing data and
 * getting the contents ready to write out the menu block.
 *
 * \param embed_dbox	True if dialogue box names should be embeded; else False.
 * \param verbose	True if verbose output is required; else False.
 * \return		True if collation completed successfully; else False.
 */

bool data_collate_structures(bool embed_dbox, bool verbose)
{
	struct menu_definition	*menu;
	struct item_definition	*item;
	struct indirection_data	*indirection;
	struct validation_data	*validation;
	struct submenu_data	*submenu;
	struct dbox_data	*dbox;
	struct dbox_chain_data	*dbox_chain;
	int			width, offset, item_offset, chain, start_length;

	if (menu_list == NULL)
		return false;

	/**
	 * Start by filling in the menu and items blocks, and collecting
	 * together preliminary details of where indirected data, validation
	 * strings, dialogue boxes and submenus will go.
	 */

	offset = sizeof(struct file_head_block);

	menu = menu_list;

	while (menu != NULL) {
		item = menu->first_item;

		/* Work out the length of the start block. */

		start_length = (strlen(menu->tag) + 12) & (~3);
		if (start_length > longest_menu_start)
			longest_menu_start = start_length;

		/* Create a dummy menu item if there isn't one. */

		if (item == NULL) {
			item = (struct item_definition *) malloc(sizeof(struct item_definition));

			if (item != NULL) {
				item->text = (char *) malloc(1);

				if (item->text == NULL) {
					free(item);
					item = NULL;
				} else {
					*(item->text) = '\0';
					item->validation = NULL;
					item->text_len = 0;

					item->file_offset = NULL_OFFSET;

					item->menu_flags = 0;
					item->icon_flags = wimp_ICON_FILLED;

					*(item->submenu_tag) = '\0';
					item->submenu_dbox = false;

					item->submenu = NULL;
					item->dbox = NULL;
					item->next_submenu = NULL_OFFSET;

					item->icon_foreground = wimp_COLOUR_BLACK;
					item->icon_background = wimp_COLOUR_WHITE;

					item->next = NULL;

					menu->first_item = item;

					(menu->items)++;
				}
			}
		}

		/* Indirect the title data. */

		if (menu->title_len > 0 && item != NULL) {
			item->menu_flags |= wimp_MENU_TITLE_INDIRECTED;

			indirection = (struct indirection_data *) malloc(sizeof(struct indirection_data));
			if (indirection != NULL) {
				indirection->menu = menu;
				indirection->item = NULL;
				indirection->next = indirection_list;
				indirection_list = indirection;
			}
		}

		/* Calculate an offset for the menu block in the file. */

		menu->file_offset = offset;
		offset += sizeof(struct file_menu_start_block) + sizeof(struct file_menu_block) + (menu->items * sizeof(struct file_item_block));

		/* Scan through the menu items. */

		width = 0;
		item_offset = menu->file_offset + sizeof(struct file_menu_start_block) + sizeof(struct file_menu_block);
		while (item != NULL) {
			/* Track the widest menu item in characters. */

			if (strlen(item->text) > width)
				width = strlen(item->text);

			/* Set the last flag for the final item in the menu. */

			if (item->next == NULL)
				item->menu_flags |= wimp_MENU_LAST;

			/* Set up icon flags. */

			item->icon_flags |= wimp_ICON_TEXT;
			item->icon_flags |= ((item->icon_foreground << wimp_ICON_FG_COLOUR_SHIFT) & wimp_ICON_FG_COLOUR);
			item->icon_flags |= ((item->icon_background << wimp_ICON_BG_COLOUR_SHIFT) & wimp_ICON_BG_COLOUR);

			/* Record the positions of dialogue boxes and submenus. */

			if (*(item->submenu_tag) != '\0') {
				if (item->submenu_dbox) {
					dbox = (struct dbox_data *) malloc(sizeof(struct dbox_data));
					item->dbox = data_find_dbox_chain_from_tag(item->submenu_tag);
					if (dbox != NULL) {
						dbox->item = item;
						dbox->next = dbox_list;
						dbox_list = dbox;

						if (item->dbox == NULL) {
							item->dbox = (struct dbox_chain_data *) malloc(sizeof(struct dbox_chain_data));
							if (item->dbox != NULL) {
								(item->dbox)->tag = item->submenu_tag; /* Point to the data in the item block. */
								(item->dbox)->first_dbox = NULL_OFFSET;
								(item->dbox)->file_offset = 0;
								(item->dbox)->block_length = 0;

								(item->dbox)->next = dbox_chain_list;
								dbox_chain_list = item->dbox;
							}
						}
					}
				} else {
					submenu = (struct submenu_data *) malloc(sizeof(struct submenu_data));
					item->submenu = data_find_menu_from_tag(item->submenu_tag);
					if (submenu != NULL) {
						submenu->item = item;
						submenu->next = submenu_list;
						submenu_list = submenu;
					}
				}
			}

			/* Record positions of indirection and validation blocks. */

			if (item->text_len > 0) {
				item->icon_flags |= wimp_ICON_INDIRECTED;

				indirection = (struct indirection_data *) malloc(sizeof(struct indirection_data));
				if (indirection != NULL) {
					indirection->menu = NULL;
					indirection->item = item;
					indirection->next = indirection_list;
					indirection_list = indirection;
				}

				if (item->validation != NULL) {
					validation = (struct validation_data *) malloc(sizeof(struct validation_data));
					if (validation != NULL) {
						validation->item = item;
						validation->string_len = strlen(item->validation) + 1;
						validation->next = validation_list;
						validation_list = validation;
					}
				}
			}

			/* Calculate an offset into the file for the item. */

			item->file_offset = item_offset;
			item_offset += sizeof(struct file_item_block);

			item = item->next;
		}

		menu->item_width = width*16 + 16;

		menu = menu->next;
	}

	/**
	 * Link up the submenu chains.
	 *
	 * The dialogue box chain(s) are left un-linked for now, as the
	 * structure will depend on the final file format.
	 */

	menu = menu_list;

	while (menu != NULL) {
		submenu = submenu_list;
		chain = NULL_OFFSET;

		while (submenu != NULL) {
			if ((submenu->item)->submenu == menu) {
				(submenu->item)->next_submenu = chain;
				chain = (submenu->item)->file_offset + 4;
			}

			submenu = submenu->next;
		}

		if (chain != NULL_OFFSET)
			menu->first_submenu = chain;

		menu = menu->next;
	}

	/**
	 * Link up the dialogue box chains in the correct format.
	 */

	if (embed_dbox) {
		dbox_chain = dbox_chain_list;

		while (dbox_chain != NULL) {
			dbox = dbox_list;
			chain = NULL_OFFSET;

			while (dbox != NULL) {
				if ((dbox->item)->dbox == dbox_chain) {
					(dbox->item)->next_submenu = chain;
					chain = (dbox->item)->file_offset + 4;
				}

				dbox = dbox->next;
			}

			if (chain != NULL_OFFSET)
				dbox_chain->first_dbox = chain;

			dbox_chain = dbox_chain->next;
		}
	} else {
		/**
		 * Build up the dialogue box list.  Linking the list backwards
		 * should ensure that the dbox order matches that generated by
		 * the older BASIC versions of MenuGen.
	 	*/

		dbox = dbox_list;
		chain = NULL_OFFSET;

		while (dbox != NULL) {
			(dbox->item)->next_submenu = chain;
			chain = (dbox->item)->file_offset + 4;

			dbox = dbox->next;
		}

		if (chain != NULL_OFFSET) {
			dbox_offset = chain;
		}
	}

	/**
	 * With the menu blocks in place, build up the indirected data block
	 * to follow it.
	 */

	indirection = indirection_list;

	while (indirection != NULL) {
		if (indirection->menu != NULL) {
			indirection->file_offset = offset;
			indirection->block_length = (((indirection->menu)->title_len) + 7) & (~3);
			indirection->target = (indirection->menu)->file_offset + 8;

			offset += indirection->block_length;
		} else if (indirection->item != NULL) {
			indirection->file_offset = offset;
			indirection->block_length = (((indirection->item)->text_len) + 7) & (~3);
			indirection->target = (indirection->item)->file_offset + 12;

			offset += indirection->block_length;
		}

		if (indirection->block_length > longest_indirection)
			longest_indirection = indirection->block_length;

		indirection = indirection->next;
	}

	if (indirection_list != NULL)
		offset += 4;

	/**
	 * Next, build up the validation string data block to follow that.
	 */

	validation = validation_list;

	while (validation != NULL) {
		if (validation->item != NULL) {
			validation->file_offset = offset;
			validation->block_length = ((validation->string_len) + 11) & (~3);
			validation->target = (validation->item)->file_offset + 16;

			offset += validation->block_length;
		}

		if (validation->block_length > longest_validation)
			longest_validation = validation->block_length;

		validation = validation->next;
	}

	if (validation_list != NULL)
		offset += 4;

	/**
	 * If we're embedding dialogue boxes, construct the data for the
	 * embedded list of tag names.
	 */

	if (embed_dbox) {
		dbox_chain = dbox_chain_list;

		offset+= 4; /* Allow space for a 0 word at the head of the list. */

		while (dbox_chain != NULL) {
			dbox_chain->file_offset = offset;
			dbox_chain->block_length = (strlen(dbox_chain->tag) + 8) & (~3);

			offset += dbox_chain->block_length;

			if (dbox_chain->block_length > longest_dbox_chain)
				longest_dbox_chain = dbox_chain->block_length;

			dbox_chain = dbox_chain->next;
		}

		if (dbox_chain_list != NULL)
			offset += 4;
	}

	return true;
}

/**
 * Return the menu block corresponding to the given tag.
 *
 * Param:  *tag		The tag to find a block for.
 * Return:		A pointer to the menu block; or NULL if not found.
 */

struct menu_definition *data_find_menu_from_tag(char *tag)
{
	struct menu_definition	*menu;

	menu = menu_list;

	while (menu != NULL && strcmp(tag, menu->tag) != 0)
		menu = menu->next;

	return menu;
}

/**
 * Return the dbox chain block corresponding to the given tag.
 *
 * Param:  *tag		The tag to find a block for.
 * Return:		A pointer to the dbox chain block; or NULL if not found.
 */

struct dbox_chain_data		*data_find_dbox_chain_from_tag(char *tag)
{
	struct dbox_chain_data	*dbox_chain;

	dbox_chain = dbox_chain_list;

	while (dbox_chain != NULL && strcmp(tag, dbox_chain->tag) != 0)
		dbox_chain = dbox_chain->next;

	return dbox_chain;
}


/**
 * Print details of the menu structures to stdout.
 *
 */

void data_print_structure_report(void)
{
	struct menu_definition	*menu;
	struct item_definition	*item;
	struct indirection_data	*indirection;
	struct validation_data	*validation;
	struct submenu_data	*submenu;
	struct dbox_data	*dbox;
	struct dbox_chain_data	*dbox_chain;

	/* Print the contents of the menu structures. */

	menu = menu_list;

	if (menu != NULL) {
		printf("================================================================================\n");
		printf("Menu Blocks\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (menu != NULL) {
		printf("Menu tag:             %s\n", menu->tag);
		printf("Title:                %s\n", menu->title);
		printf("Indirected:           %s\n", data_boolean_yes_no(menu->title_len > 0));
		if (menu->title_len > 0)
			printf("Indirected length:    %d bytes\n", menu->title_len);
		printf("Reversed:             %s\n", data_boolean_yes_no(menu->reversed));
		printf("Item width:           %d OS units\n", menu->item_width);
		printf("Item height:          %d OS units\n", menu->item_height);
		printf("Item gap:             %d OS units\n", menu->item_gap);
		printf("Title foreground:     Colour %d\n", menu->title_foreground);
		printf("Title background:     Colour %d\n", menu->title_background);
		printf("Work Area foreground: Colour %d\n", menu->work_area_foreground);
		printf("Work Area foreground: Colour %d\n", menu->work_area_background);
		printf("File block offset:    %d bytes\n", menu->file_offset);
		printf("Items:                %d\n", menu->items);

		item = menu->first_item;

		while (item != NULL) {
			printf("  ------------------------------------------------------------------------------\n");
			printf("  Item text:          %s\n", item->text);
			printf("  Indirected:         %s\n", data_boolean_yes_no(item->text_len > 0));
			if (item->text_len > 0)
				printf("  Indirected length:  %d bytes\n", item->text_len);
			if (item->validation != NULL)
				printf("  Validation string:  %s\n", item->validation);
			if (*(item->submenu_tag) != '\0') {
				if (item->submenu_dbox) {
					printf ("  Dialogue box:       %s\n", item->submenu_tag);
				} else {
					printf ("  Submenu:            %s (%s)\n", item->submenu_tag, (item->submenu)->title);
				}
			}
			printf("  Ticked:             %s\n", data_boolean_yes_no(item->menu_flags & wimp_MENU_TICKED));
			printf("  Dotted:             %s\n", data_boolean_yes_no(item->menu_flags & wimp_MENU_SEPARATE));
			printf("  Shaded:             %s\n", data_boolean_yes_no(item->icon_flags & wimp_ICON_SHADED));
			printf("  Writable:           %s\n", data_boolean_yes_no(item->menu_flags & wimp_MENU_WRITABLE));
			printf("  Sprite:             %s\n", data_boolean_yes_no(item->icon_flags & wimp_ICON_SPRITE));
			if (item->icon_flags & wimp_ICON_SPRITE)
				printf("  Half size:          %s\n", data_boolean_yes_no(item->menu_flags & wimp_ICON_HALF_SIZE));
			printf("  Submenu message:    %s\n", data_boolean_yes_no(item->menu_flags & wimp_MENU_GIVE_WARNING));
			printf("  Always open:        %s\n", data_boolean_yes_no(item->menu_flags & wimp_MENU_SUB_MENU_WHEN_SHADED));
			printf("  Item foreground:    Colour %d\n", item->icon_foreground);
			printf("  Item background:    Colour %d\n", item->icon_background);
			printf("  File block offset:  %d bytes\n", item->file_offset);

			item = item->next;
		}

		printf("--------------------------------------------------------------------------------\n");

		menu = menu->next;
	}

	/* Print out the list of submenu links. */

	submenu = submenu_list;

	if (submenu != NULL) {
		printf("================================================================================\n");
		printf("Submenu References\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (submenu != NULL) {
		if (submenu->item != NULL) {
			printf("Item text:            %s\n", (submenu->item)->text);
			printf("Submenu tag:          %s\n", (submenu->item)->submenu_tag);
		}

		printf("--------------------------------------------------------------------------------\n");

		submenu = submenu->next;
	}

	/* Print the contents of the dialogue box chain. */

	dbox_chain = dbox_chain_list;

	if (dbox_chain != NULL) {
		printf("================================================================================\n");
		printf("Dialogue Box Chain\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (dbox_chain != NULL) {
		printf("Box tag:              %s\n", dbox_chain->tag);
		printf("First target offset:  %d bytes\n", dbox_chain->first_dbox);
		printf("Block Length in file: %d bytes\n", dbox_chain->block_length);
		printf("File block offset:    %d bytes\n", dbox_chain->file_offset);

		printf("--------------------------------------------------------------------------------\n");

		dbox_chain = dbox_chain->next;
	}

	/* Print out the list of dbox linls. */

	dbox = dbox_list;

	if (dbox != NULL) {
		printf("================================================================================\n");
		printf("Dialogue Box References\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (dbox != NULL) {
		if (dbox->item != NULL) {
			printf("Item text:            %s\n", (dbox->item)->text);
			printf("DBox tag:             %s\n", (dbox->item)->submenu_tag);
		}

		printf("--------------------------------------------------------------------------------\n");

		dbox = dbox->next;
	}

	/* Print the indirection blocks. */

	indirection = indirection_list;

	if (indirection != NULL) {
		printf("================================================================================\n");
		printf("Indirected Data Blocks\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (indirection != NULL) {
		if (indirection->menu != NULL) {
			printf("Menu title:           %s\n", (indirection->menu)->title);
			printf("Maximum length:       %d bytes\n", (indirection->menu)->title_len);
		} else if (indirection->item != NULL) {
			printf("Item text:            %s\n", (indirection->item)->text);
			printf("Maximum length:       %d bytes\n", (indirection->item)->text_len);
		}

		printf("Target offset:        %d bytes\n", indirection->target);
		printf("Block Length in file: %d bytes\n", indirection->block_length);
		printf("File block offset:    %d bytes\n", indirection->file_offset);

		printf("--------------------------------------------------------------------------------\n");

		indirection = indirection->next;
	}

	/* Print the validation blocks. */

	validation = validation_list;

	if (validation != NULL) {
		printf("================================================================================\n");
		printf("Validation Strings\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	while (validation != NULL) {
		if (validation->item != NULL) {
			printf("Validation string:    %s\n", (validation->item)->validation);
		}

		printf("String length:        %d bytes\n", validation->string_len);
		printf("Target offset:        %d bytes\n", validation->target);
		printf("Block Length in file: %d bytes\n", validation->block_length);
		printf("File block offset:    %d bytes\n", validation->file_offset);

		printf("--------------------------------------------------------------------------------\n");

		validation = validation->next;
	}

	printf("================================================================================\n");
}

/**
 * Write a menu definition file.
 *
 * \param *filename	The file to write.
 * \return		True if the file was created OK; else False;
 */

bool data_write_standard_menu_file(char *filename)
{
	FILE				*file;

	int				offset;

	struct menu_definition		*menu;
	struct item_definition		*item;
	struct indirection_data		*indirection;
	struct validation_data		*validation;
	struct dbox_data		*dbox, *tail;
	struct dbox_chain_data		*dbox_chain;

	struct file_head_block		head_block;
	struct file_menu_block		menu_block;
	struct file_item_block		item_block;
	struct file_dialogue_head_block	dbox_head_block;

	struct file_menu_start_block	*menu_start_block = NULL;
	struct file_indirection_block	*indirection_block = NULL;
	struct file_validation_block	*validation_block = NULL;
	struct file_dialogue_tag_block	*dbox_tag_block = NULL;

	menu_start_block = malloc(longest_menu_start);
	indirection_block = malloc(longest_indirection);
	validation_block = malloc(longest_validation);
	dbox_tag_block = malloc(longest_dbox_chain);

	if (menu_start_block == NULL || indirection_block == NULL || validation_block == NULL || dbox_tag_block == NULL) {
		if (menu_start_block != NULL)
			free(menu_start_block);
		if (indirection_block != NULL)
			free(indirection_block);
		if (validation_block != NULL)
			free(validation_block);
		if (dbox_tag_block != NULL)
			free(dbox_tag_block);

		return false;
	}

	file = fopen(filename, "w");

	if (file == NULL)
		return false;

	/* Write the file header. */

	/* If there is a dbox_chain and the first item has a non-zero file
	 * offset, then we're using the embedded format.  The offset in the
	 * header is a word ahead of the first block, and points to the
	 * leading zero.
	 */

	if (dbox_chain_list != NULL && dbox_chain_list->file_offset != 0)
		head_block.dialogues = dbox_chain_list->file_offset - 4;
	else
		head_block.dialogues = dbox_offset;

	if (indirection_list != NULL)
		head_block.indirection = indirection_list->file_offset;
	else
		head_block.indirection = NULL_OFFSET;

	if (validation_list != NULL)
		head_block.validation = validation_list->file_offset;
	else
		head_block.validation = NULL_OFFSET;

	fwrite(&head_block, sizeof(struct file_head_block), 1, file);

	/* Write the menu & item blocks. */

	menu = menu_list;

	while (menu != NULL) {
		if (menu->next == NULL)
			menu_start_block->next = NULL_OFFSET;
		else
			menu_start_block->next = (menu->next)->file_offset + 8;

		menu_start_block->submenus = menu->first_submenu;

		if (menu->title_len == 0) {
			strncpy(menu_block.title_data.text, menu->title, 12);
		} else {
			menu_block.title_data.indirected_text.indirection = 0;
			menu_block.title_data.indirected_text.validation = -1;
			menu_block.title_data.indirected_text.size = menu->title_len;
		}

		menu_block.title_fg = (wimp_colour) menu->title_foreground;
		menu_block.title_bg = (wimp_colour) menu->title_background;
		menu_block.work_fg = (wimp_colour) menu->work_area_foreground;
		menu_block.work_bg = (wimp_colour) menu->work_area_background;
		menu_block.width = menu->item_width;
		menu_block.height = menu->item_height;
		menu_block.gap = menu->item_gap;

		fwrite(menu_start_block, 8, 1, file);
		fwrite(&menu_block, sizeof(struct file_menu_block), 1, file);

		item = menu->first_item;

		while (item != NULL) {
			if (item->text_len == 0) {
				strncpy(item_block.icon_data.text, item->text, 12);
			} else {
				item_block.icon_data.indirected_text.indirection = 0;
				item_block.icon_data.indirected_text.validation = -1;
				item_block.icon_data.indirected_text.size = item->text_len;
			}

			item_block.menu_flags = item->menu_flags;
			item_block.icon_flags = item->icon_flags;
			item_block.submenu_file_offset = item->next_submenu;

			fwrite(&item_block, sizeof(struct file_item_block), 1, file);

			item = item->next;
		}

		menu = menu->next;
	}

	/* Write the indirected data blocks. */

	indirection = indirection_list;

	while (indirection != NULL) {
		indirection_block->location = indirection->target;
		if (indirection->menu != NULL)
			strncpy(indirection_block->data, (indirection->menu)->title, indirection->block_length - sizeof(struct file_indirection_block));
		else if (indirection->item != NULL)
			strncpy(indirection_block->data, (indirection->item)->text, indirection->block_length - sizeof(struct file_indirection_block));
		else
			strncpy(indirection_block->data, "", indirection->block_length - sizeof(struct file_indirection_block));

		fwrite(indirection_block, indirection->block_length, 1, file);

		indirection = indirection->next;
	}

	if (indirection_list != NULL) {
		indirection_block->location = NULL_OFFSET;
		fwrite(indirection_block, 4, 1, file);
	}

	/* Write the validation string blocks. */

	validation = validation_list;

	while (validation != NULL) {
		validation_block->location = validation->target;
		validation_block->length = validation->block_length;
		if (validation->item != NULL && (validation->item)->validation != NULL)
			strncpy(validation_block->data, (validation->item)->validation, validation->block_length - sizeof(struct file_validation_block));
		else
			strncpy(validation_block->data, "", validation->block_length - sizeof(struct file_validation_block));

		fwrite(validation_block, validation->block_length, 1, file);

		validation = validation->next;
	}

	if (validation_list != NULL) {
		validation_block->location = NULL_OFFSET;
		fwrite(validation_block, 4, 1, file);
	}

	/* Write the dialogue data blocks. */

	if (dbox_chain_list != NULL && dbox_chain_list->file_offset != 0) {
		dbox_head_block.zero = 0;
		fwrite(&dbox_head_block, 4, 1, file);

		dbox_chain = dbox_chain_list;

		while (dbox_chain != NULL) {
			dbox_tag_block->dialogues = dbox_chain->first_dbox;
			if (dbox_chain->tag != NULL)
				strncpy(dbox_tag_block->tag, dbox_chain->tag, dbox_chain->block_length - sizeof(struct file_dialogue_tag_block));
			else
				strncpy(dbox_tag_block->tag, "", dbox_chain->block_length - sizeof(struct file_dialogue_tag_block));

			fwrite(dbox_tag_block, dbox_chain->block_length, 1, file);

			dbox_chain = dbox_chain->next;
		}

		dbox_tag_block->dialogues = NULL_OFFSET;
		fwrite(dbox_tag_block, 4, 1, file);
	}

	fclose(file);

	free(menu_start_block);
	free(indirection_block);
	free(validation_block);
	free(dbox_tag_block);

	/* Output dialogue box details. */

	if (dbox_chain_list != NULL && dbox_chain_list->file_offset != 0) {
		printf("Dialogue box tags embedded into file.\n");
	} else if (dbox_list != NULL) {
		printf("Dialogue boxes required in order:\n");

		/**
		 * This is messy, as the list must be printed in reverse order
		 * so that it is compatible with the way that the original
		 * BASIC versions of MenuGen worked.
		 */

		tail = NULL;
		offset = 0;

		while (tail != dbox_list) {
			dbox = dbox_list;

			while (dbox != NULL && dbox->next != tail)
				dbox = dbox->next;

			printf("%4d : %s\n", 4*offset++, (dbox->item)->submenu_tag);

			tail = dbox;
		}
	}

	/* Output the list of menus in data block order. */

	printf("Menus created in order:\n");

	menu = menu_list;
	offset = 0;

	while (menu != NULL) {
		printf("%4d : %s (%s)\n", 4*offset++, menu->tag, menu->title);
		menu = menu->next;
	}

	return true;
}


/**
 * Create a new menu, giving it the supplied tag and title and making it the
 * current menu.
 *
 * \param *tag		The internal tag used to identify the menu.
 * \param *title	The menu title.
 * \return		True if the menu created OK; else False.
 */

bool data_create_new_menu(char *tag, char *title)
{
	struct menu_definition	*menu;

	/* Allocate storage and get out if we fail. */

	menu = (struct menu_definition *) malloc(sizeof(struct menu_definition));

	if (menu == NULL)
		return false;

	menu->title = (char *) malloc(strlen(title)+1);

	if (menu->title == NULL) {
		free(menu);
		return false;
	}

	strcpy(menu->tag, tag);
	strcpy(menu->title, title);

	if (strlen(title) > 12)
		menu->title_len = strlen(title) + 1;
	else
		menu->title_len = 0;

	menu->items = 0;
	menu->first_item = NULL;
	menu->file_offset = NULL_OFFSET;

	menu->reversed = false;

	menu->item_width = 0;
	menu->item_height = 44;
	menu->item_gap = 0;

	menu->title_foreground = wimp_COLOUR_BLACK;
	menu->title_background = wimp_COLOUR_LIGHT_GREY;
	menu->work_area_foreground = wimp_COLOUR_BLACK;
	menu->work_area_background = wimp_COLOUR_WHITE;

	menu->first_submenu = NULL_OFFSET;

	menu->next = NULL;

	if (current_menu != NULL)
		current_menu->next = menu;
	else
		menu_list = menu;

	current_menu = menu;
	current_item = NULL;

	return true;
}

/**
 * Create a new menu item in the current menu, giving it the supplied title
 * and making it the current menu item.
 *
 * \param *title	The menu item title.
 * \return		True if the item was created OK; else False.
 */

bool data_create_new_item(char *text)
{
	struct item_definition	*item;

	/* If there isn't a current menu, then we can't create a new item. */

	if (current_menu == NULL)
		return false;

	item = (struct item_definition *) malloc(sizeof(struct item_definition));

	if (item == NULL)
		return false;

	item->text = (char *) malloc(strlen(text)+1);

	if (item->text == NULL) {
		free(item);
		return false;
	}

	strcpy(item->text, text);
	item->validation = NULL;

	if (strlen(text) > 12)
		item->text_len = strlen(text) + 1;
	else
		item->text_len = 0;

	item->file_offset = NULL_OFFSET;

	item->menu_flags = 0;
	item->icon_flags = wimp_ICON_FILLED;

	*(item->submenu_tag) = '\0';
	item->submenu_dbox = false;

	item->submenu = NULL;
	item->dbox = NULL;
	item->next_submenu = NULL_OFFSET;

	item->icon_foreground = wimp_COLOUR_BLACK;
	item->icon_background = wimp_COLOUR_WHITE;

	item->next = NULL;

	if (current_item != NULL)
		current_item->next = item;
	else
		current_menu->first_item = item;

	current_item = item;
	(current_menu->items)++;

	return true;
}

/**
 * Set the current item's submenu status.
 *
 * \param *tag		The tag for the submenu.
 * \param dbox		True if the item is a dbox; else False.
 * \return		True if the tag was set correctly; else False.
 */

bool data_set_item_submenu(char *tag, bool dbox)
{
	if (current_item == NULL)
		return false;

	if (strlen(tag)+1 > MAX_TAG_LEN)
		return false;

	strcpy(current_item->submenu_tag, tag);
	current_item->submenu_dbox = dbox;

	return true;
}

/**
 * Set the current menu's title indirection status.
 *
 * \param size		The indirected buffer size.
 * \return		True if the indirection was set correctly; else False.
 */

bool data_set_menu_title_indirection(int size)
{
	if (current_menu == NULL)
		return false;

	if (size >= current_menu->title_len)
		current_menu->title_len = size + 1;

	return true;
}

/**
 * Set the current item's indirection status.
 *
 * \param size		The indirected buffer size.
 * \return		True if the indirection was set correctly; else False.
 */

bool data_set_item_indirection(int size)
{
	if (current_item == NULL)
		return false;

	if (size >= current_item->text_len)
		current_item->text_len = size + 1;

	return true;
}

/**
 * Make the current item writable.
 *
 * \return		True if the writable status was set correctly; else False.
 */

bool data_set_item_writable(void)
{
	if (current_item == NULL)
		return false;

	data_set_item_indirection(12);
	current_item->menu_flags |= wimp_MENU_WRITABLE;

	return true;
}

/**
 * Set the current item's validation string.
 *
 * \param *validation	The validation string.
 * \return		True if the validation string was set correctly; else False.
 */

bool data_set_item_validation(char *validation)
{
	if ((current_item == NULL) ||
			((current_item->menu_flags & wimp_MENU_WRITABLE) == 0) ||
			(current_item->validation != NULL))
		return false;

	current_item->validation = (char *) malloc(strlen(validation) + 1);

	if (current_item->validation == NULL) {
		return false;
	}

	strcpy(current_item->validation, validation);

	return true;

}

/**
 * Set the current menu's colours.
 *
 * \param title_fg
 * \param title_bg
 * \param work_fg
 * \param work_bg
 * \return		True if the colours were set correctly; else False.
 */

bool data_set_menu_colours(int title_fg, int title_bg, int work_fg, int work_bg)
{
	if (current_menu == NULL)
		return false;

	current_menu->title_foreground = title_fg;
	current_menu->title_background = title_bg;
	current_menu->work_area_foreground = work_fg;
	current_menu->work_area_background = work_bg;

	return true;
}

/**
 * Set the current item's colours.
 *
 * \param title_fg
 * \param title_bg
 * \param work_fg
 * \param work_bg
 * \return		True if the colours were set correctly; else False.
 */

bool data_set_item_colours(int icon_fg, int icon_bg)
{
	if (current_item == NULL)
		return false;

	current_item->icon_foreground = icon_fg;
	current_item->icon_background = icon_bg;

	return true;
}

/**
 * Set the current item to be reversed.
 *
 * \return		True if the state was set correctly; else False.
 */

bool data_set_menu_reversed(void)
{
	if (current_menu == NULL)
		return false;

	current_menu->reversed = true;

	return true;
}

/**
 * Set the current menu's item height.
 *
 * \param height	The new height.
 * \return		True if the height was set correctly; else False.
 */

bool data_set_menu_item_height(int height)
{
	if (current_menu == NULL)
		return false;

	current_menu->item_height = height;

	return true;
}

/**
 * Set the current menu's item gap.
 *
 * \param gap		The new gap.
 * \return		True if the gap was set correctly; else False.
 */

bool data_set_menu_item_gap(int gap)
{
	if (current_menu == NULL)
		return false;

	current_menu->item_gap = gap;

	return true;
}

/**
 * Set the current item to be ticked.
 *
 * \return		True if the status was set correctly; else False.
 */

bool data_set_item_ticked(void)
{
	if (current_item == NULL)
		return false;

	current_item->menu_flags |= wimp_MENU_TICKED;

	return true;
}

/**
 * Set the current item to be dotted.
 *
 * \return		True if the status was set correctly; else False.
 */

bool data_set_item_dotted(void)
{
	if (current_item == NULL)
		return false;

	current_item->menu_flags |= wimp_MENU_SEPARATE;

	return true;
}

/**
 * Set the current item to give a submenu warning.
 *
 * \return		True if the status was set correctly; else False.
 */

bool data_set_item_warning(void)
{
	if (current_item == NULL)
		return false;

	current_item->menu_flags |= wimp_MENU_GIVE_WARNING;

	return true;
}

/**
 * Set the current item to be available when shaded.
 *
 * \return		True if the status was set correctly; else False.
 */

bool data_set_item_when_shaded(void)
{
	if (current_item == NULL)
		return false;

	current_item->menu_flags |= wimp_MENU_SUB_MENU_WHEN_SHADED;

	return true;
}

/**
 * Set the current item to be shaded.
 *
 * \return		True if the status was set correctly; else False.
 */

bool data_set_item_shaded(void)
{
	if (current_item == NULL)
		return false;

	current_item->icon_flags |= wimp_ICON_SHADED;

	return true;
}


/**
 * Return a pointer to "Yes" or "No" depending upon the boolean state
 * of value.
 *
 * Param:		Boolean value to translate into text.
 * Return:		"Yes" or "No".
 */

static char *data_boolean_yes_no(int value)
{
	return (value) ? "Yes" : "No";
}
