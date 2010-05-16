/* MenuGen -- Data.c
 *
 * Copyright Stephen Fryatt, 2010
 *
 * Handle and track menu definition data.
 */


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

	char			submenu_tag[MAX_TAG_LEN];
	int			submenu_dbox; /* 1 if the item is a dbox. */

	struct item_definition	*next;
};

struct menu_definition {
	char			tag[MAX_TAG_LEN];
	char			*title;
	int			title_len; /* 0 for non-indirected. */

	int			items;
	struct item_definition	*first_item;

	int			file_offset; /* Where this menu resides. */

	struct menu_definition	*next;
};

static struct menu_definition	*menu_list = NULL;

static struct menu_definition	*current_menu = NULL;
static struct item_definition	*current_item = NULL;

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

	if (strlen(title) > 11)
		menu->title_len = strlen(title) + 1;
	else
		menu->title_len = 0;

	menu->items = 0;
	menu->first_item = NULL;
	menu->file_offset = NULL_OFFSET;

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

	strcpy(item->text, text);
	item->validation = NULL;

	if (strlen(text) > 11)
		item->text_len = strlen(text) + 1;
	else
		item->text_len = 0;

	item->menu_flags = 0;
	item->icon_flags = 0;

	*(item->submenu_tag) = '\0';
	item->submenu_dbox = 0;

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

	if (current_menu->title_len == 0 || size > current_menu->title_len)
		current_menu->title_len = size;

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

	if (current_item->text_len == 0 || size > current_item->text_len)
		current_item->text_len = size;

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
