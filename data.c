/* MenuGen -- Data.c
 *
 * Copyright Stephen Fryatt, 2010
 *
 * Handle and track menu definition data.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "data.h"

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
	int			submenu_dbox; /* 1 if the item is a dbox. */

	struct menu_definition	*submenu;

	int			next_submenu;

	int			file_offset; /* Where this menu item resides. */

	struct item_definition	*next;
};

struct menu_definition {
	char			tag[MAX_TAG_LEN];
	char			*title;
	int			title_len; /* 0 for non-indirected. */

	int			reversed;

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

/**
 * Standard file data structures, used to build the standard menu file.
 */

struct file_head_block {
	int		dialogues;
	int		indirection;
	int		validation;
};

struct file_menu_block {
	int		next;
	int		submenus;
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


static struct menu_definition	*menu_list = NULL;
static struct indirection_data	*indirection_list = NULL;
static struct validation_data	*validation_list = NULL;
static struct submenu_data	*submenu_list = NULL;
static struct dbox_data		*dbox_list = NULL;

static struct menu_definition	*current_menu = NULL;
static struct item_definition	*current_item = NULL;

static int			dbox_offset = NULL_OFFSET;

static int			longest_indirection = 0;
static int			longest_validation = 0;

struct menu_definition		*data_find_menu_from_tag(char *tag);
char				*data_boolean_yes_no(int value);

/**
 * Go through the assembled menu structures, filling in the missing data and
 * getting the contents ready to write out the menu block.
 *
 * Param:  verbose	1 if verbose output is required; else 0.
 * Return:		0 if collation completed successfully; else 0.
 */

int data_collate_structures(int verbose)
{
	struct menu_definition	*menu;
	struct item_definition	*item;
	struct indirection_data	*indirection;
	struct validation_data	*validation;
	struct submenu_data	*submenu;
	struct dbox_data	*dbox;
	int			width, offset, item_offset, chain;

	if (menu_list == NULL)
		return 1;

	/**
	 * Start by filling in the menu and items blocks, and collecting
	 * together preliminary details of where indirected data, validation
	 * strings, dialogue boxes and submenus will go.
	 */

	offset = sizeof(struct file_head_block);

	menu = menu_list;

	while (menu != NULL) {
		item = menu->first_item;

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
					item->submenu_dbox = 0;

					item->submenu = NULL;
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
		offset += sizeof(struct file_menu_block) + (menu->items * sizeof(struct file_item_block));

		/* Scan through the menu items. */

		width = 0;
		item_offset = menu->file_offset + sizeof(struct file_menu_block);
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
					if (dbox != NULL) {
						dbox->item = item;
						dbox->next = dbox_list;
						dbox_list = dbox;
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
	 * Build up the dialogue box list.  Linking the list backwards should
	 * ensure that the dbox order matches that generated by the older
	 * BASIC versions of MenuGen.
	 *
	 * *** This may change as new file formats are added! ***
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

	/**
	 * Next, build up the validation string data block to follow that.
	 */

	validation = validation_list;

	while (validation != NULL) {
		if (validation->item != NULL) {
			validation->file_offset = offset;
			validation->block_length = ((validation->string_len) + 11) & (~3);
			validation->target = (validation->item)->file_offset + 16;

			offset += indirection->block_length;
		}

		if (validation->block_length > longest_validation)
			longest_validation = validation->block_length;

		validation = validation->next;
	}

	return 0;
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

	menu = menu_list;

	if (menu != NULL)
		printf("--------------------------------------------------------------------------------\n");

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
}

/**
 * Write a menu definition file.
 *
 * Param:  *filename	The file to write.
 * Return:		0 if the file was created OK; else 1;
 */

int data_write_standard_menu_file(char *filename)
{
	FILE				*file;

	int				offset;

	struct menu_definition		*menu;
	struct item_definition		*item;
	struct indirection_data		*indirection;
	struct validation_data		*validation;
	struct dbox_data		*dbox, *tail;

	struct file_head_block		head_block;
	struct file_menu_block		menu_block;
	struct file_item_block		item_block;

	struct file_indirection_block	*indirection_block;
	struct file_validation_block	*validation_block;

	indirection_block = (struct file_indirection_block *) malloc(longest_indirection);
	validation_block = (struct file_validation_block *) malloc(longest_validation);

	if (indirection_block == NULL || validation_block == NULL) {
		if (indirection_block != NULL)
			free(indirection_block);
		if (validation_block != NULL)
			free(validation_block);

		return 1;
	}

	file = fopen(filename, "w");

	if (file == NULL)
		return 1;

	/* Write the file header. */

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
			menu_block.next = NULL_OFFSET;
		else
			menu_block.next = (menu->next)->file_offset + 8;

		menu_block.submenus = menu->first_submenu;

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
	// \TODO

	fclose(file);

	free(indirection_block);
	free(validation_block);

	/* Output dialogue box details. */

	if (dbox_list != NULL) {
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

	return 0;
}


/**
 * Create a new menu, giving it the supplied tag and title and making it the
 * current menu.
 *
 * Param:  *tag		The internal tag used to identify the menu.
 * Param:  *title	The menu title.
 * Return:		0 if the menu created OK; else 1.
 */

int data_create_new_menu(char *tag, char *title)
{
	struct menu_definition	*menu;

	/* Allocate storage and get out if we fail. */

	menu = (struct menu_definition *) malloc(sizeof(struct menu_definition));

	if (menu == NULL)
		return 1;

	menu->title = (char *) malloc(strlen(title)+1);

	if (menu->title == NULL) {
		free(menu);
		return 1;
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

	menu->reversed = 0;

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

	return 0;
}

/**
 * Create a new menu item in the current menu, giving it the supplied title
 * and making it the current menu item.
 *
 * Param:  *title	The menu item title.
 * Return:		0 if the item was created OK; else false.
 */

int data_create_new_item(char *text)
{
	struct item_definition	*item;

	/* If there isn't a current menu, then we can't create a new item. */

	if (current_menu == NULL)
		return 1;

	item = (struct item_definition *) malloc(sizeof(struct item_definition));

	if (item == NULL)
		return 1;

	item->text = (char *) malloc(strlen(text)+1);

	if (item->text == NULL) {
		free(item);
		return 1;
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
	item->submenu_dbox = 0;

	item->submenu = NULL;
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

	return 0;
}

/**
 * Set the current item's submenu status.
 *
 * Param:  *tag		The tag for the submenu.
 * Param:  dbox		1 if the item is a dbox; else 0.
 * Return:		0 if the tag was set correctly.
 */

int data_set_item_submenu(char *tag, int dbox)
{
	if (current_item == NULL)
		return 1;

	if (strlen(tag)+1 > MAX_TAG_LEN)
		return 1;

	strcpy(current_item->submenu_tag, tag);
	current_item->submenu_dbox = dbox;

	return 0;
}

/**
 * Set the current menu's title indirection status.
 *
 * Param:  size		The indirected buffer size.
 * Return:		0 if the indirection was set correctly.
 */

int data_set_menu_title_indirection(int size)
{
	if (current_menu == NULL)
		return 1;

	if (size >= current_menu->title_len)
		current_menu->title_len = size + 1;

	return 0;
}

/**
 * Set the current item's indirection status.
 *
 * Param:  size		The indirected buffer size.
 * Return:		0 if the indirection was set correctly.
 */

int data_set_item_indirection(int size)
{
	if (current_item == NULL)
		return 1;

	if (size >= current_item->text_len)
		current_item->text_len = size + 1;

	return 0;
}

/**
 * Make the current item writable.
 *
 * Return:		0 if the writable status was set correctly.
 */

int data_set_item_writable(void)
{
	if (current_item == NULL)
		return 1;

	data_set_item_indirection(12);
	current_item->menu_flags |= wimp_MENU_WRITABLE;

	return 0;
}

/**
 * Set the current item's validation string.
 *
 * Param:  *validation	The validation string.
 * Return:		0 of the validation string was set correctly.
 */

int data_set_item_validation(char *validation)
{
	if ((current_item == NULL) ||
			((current_item->menu_flags & wimp_MENU_WRITABLE) == 0) ||
			(current_item->validation != NULL))
		return 1;

	current_item->validation = (char *) malloc(strlen(validation)+1);

	if (current_item->validation == NULL) {
		return 1;
	}

	strcpy(current_item->validation, validation);

	return 0;

}

/**
 * Set the current menu's colours.
 *
 * Param:  title_fg
 * Param:  title_bg
 * Param:  work_fg
 * Param:  work_bg
 * Return:		0 if the colours were set correctly.
 */

int data_set_menu_colours(int title_fg, int title_bg, int work_fg, int work_bg)
{
	if (current_menu == NULL)
		return 1;

	current_menu->title_foreground = title_fg;
	current_menu->title_background = title_bg;
	current_menu->work_area_foreground = work_fg;
	current_menu->work_area_background = work_bg;

	return 0;
}

/**
 * Set the current item's colours.
 *
 * Param:  title_fg
 * Param:  title_bg
 * Param:  work_fg
 * Param:  work_bg
 * Return:		0 if the colours were set correctly.
 */

int data_set_item_colours(int icon_fg, int icon_bg)
{
	if (current_item == NULL)
		return 1;

	current_item->icon_foreground = icon_fg;
	current_item->icon_background = icon_bg;

	return 0;
}

/**
 * Set the current item to be reversed.
 *
 * Return:		0 if the state was set correctly.
 */

int data_set_menu_reversed(void)
{
	if (current_menu == NULL)
		return 1;

	current_menu->reversed = 1;

	return 0;
}

/**
 * Set the current menu's item height.
 *
 * Param:  height	The new height.
 * Return:		0 if the height was set correctly.
 */

int data_set_menu_item_height(int height)
{
	if (current_menu == NULL)
		return 1;

	current_menu->item_height = height;

	return 0;
}

/**
 * Set the current menu's item gap.
 *
 * Param:  gap		The new gap.
 * Return:		0 if the gap was set correctly.
 */

int data_set_menu_item_gap(int gap)
{
	if (current_menu == NULL)
		return 1;

	current_menu->item_gap = gap;

	return 0;
}

/**
 * Set the current item to be ticked.
 *
 * Return:		0 if the status was set correctly.
 */

int data_set_item_ticked(void)
{
	if (current_item == NULL)
		return 1;

	current_item->menu_flags |= wimp_MENU_TICKED;

	return 0;
}

/**
 * Set the current item to be dotted.
 *
 * Return:		0 if the status was set correctly.
 */

int data_set_item_dotted(void)
{
	if (current_item == NULL)
		return 1;

	current_item->menu_flags |= wimp_MENU_SEPARATE;

	return 0;
}

/**
 * Set the current item to give a submenu warning.
 *
 * Return:		0 if the status was set correctly.
 */

int data_set_item_warning(void)
{
	if (current_item == NULL)
		return 1;

	current_item->menu_flags |= wimp_MENU_GIVE_WARNING;

	return 0;
}

/**
 * Set the current item to be available when shaded.
 *
 * Return:		0 if the status was set correctly.
 */

int data_set_item_when_shaded(void)
{
	if (current_item == NULL)
		return 1;

	current_item->menu_flags |= wimp_MENU_SUB_MENU_WHEN_SHADED;

	return 0;
}

/**
 * Set the current item to be shaded.
 *
 * Return:		0 if the status was set correctly.
 */

int data_set_item_shaded(void)
{
	if (current_item == NULL)
		return 1;

	current_item->icon_flags |= wimp_ICON_SHADED;

	return 0;
}


/**
 * Return a pointer to "Yes" or "No" depending upon the boolean state
 * of value.
 *
 * Param:		Boolean value to translate into text.
 * Return:		"Yes" or "No".
 */

char *data_boolean_yes_no(int value)
{
	return (value) ? "Yes" : "No";
}
