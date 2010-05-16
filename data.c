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

	int			file_offset; /* Where this menu resides. */

	struct menu_definition	*next;
};

struct indirection_data {
	struct menu_definition	*menu;
	struct item_definition	*item;

	int			file_offset;
	int			block_length;

	struct indirection_data	*next;
};

struct validation_data {
	struct item_definition	*item;

	int			string_len;

	int			file_offset;
	int			block_length;

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
	int		indirected;
	int		validation;
};

struct file_menu_block {
	int		next;
	int		submenus;
	union {
		char		text [12];
		struct {
			char		*text;
			byte		reserved [8];
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
	wimp_icon_data	data;
};

struct file_indirection_block {
	int		location;
	char		data[];
};

struct file_validation_block {
	int		location;
	int		length;
	char		data[];
};


static struct menu_definition	*menu_list = NULL;
static struct indirection_data	*indirection_list = NULL;
static struct validation_data	*validation_list = NULL;
static struct submenu_data	*submenu_list = NULL;
static struct dbox_data		*dbox_list = NULL;

static struct menu_definition	*current_menu = NULL;
static struct item_definition	*current_item = NULL;

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
	int			width, offset, item_offset;

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

		if (verbose) {
			printf("Menu: '%s'\n", menu->tag);
			printf("  Title: '%s'\n", menu->title);
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

			if (verbose)
				printf("    Indirected title length: %d\n", menu->title_len);
		}

		/* Calculate an offset for the menu block in the file. */

		menu->file_offset = offset;
		offset += sizeof(struct file_menu_block) + (menu->items * sizeof(struct file_item_block));

		if (verbose) {
			printf("  Menu block at file offset: %d\n", menu->file_offset);
			printf("  Menu items: %d\n", menu->items);
		}

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
					if (submenu != NULL) {
						submenu->item = item;
						submenu->next = submenu_list;
						submenu_list = submenu;
					}
				}
			}

			/* Record positions of indirection and validation blocks. */

			if (item->text_len > 0) {
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

			if (verbose)
				printf("  Item block at file offset: %d\n", item->file_offset);

			item = item->next;
		}

		menu->item_width = width*16 + 16;
		if (verbose)
			printf ("  Width: %d units (%d characters)\n", menu->item_width, width);

		menu = menu->next;
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

			offset += indirection->block_length;

			if (verbose)
				printf("Menu title indirection for %d bytes (block length %d bytes) at file offset: %d\n",
					(indirection->menu)->title_len, indirection->block_length, indirection->file_offset);
		} else if (indirection->item != NULL) {
			indirection->file_offset = offset;
			indirection->block_length = (((indirection->item)->text_len) + 7) & (~3);

			offset += indirection->block_length;

			if (verbose)
				printf("Menu item indirection for %d bytes (block length %d bytes) at file offset: %d\n",
					(indirection->item)->text_len, indirection->block_length, indirection->file_offset);
		}

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

			offset += indirection->block_length;

			if (verbose)
				printf("Menu item validation for %d bytes (block length %d bytes) at file offset: %d\n",
					validation->string_len, validation->block_length, validation->file_offset);
		}
	}


	return 0;
}


/**
 * Create a new menu, giving it the supplied tag and title and making it the
 * current menu.
 *
 * Param:  *tag		The internal tag used to identify the menu.
 * Param:  *title	The menu title.
 * Return:		0 if the menu created OK; else false.
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
	item->icon_flags = 0;

	*(item->submenu_tag) = '\0';
	item->submenu_dbox = 0;

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

