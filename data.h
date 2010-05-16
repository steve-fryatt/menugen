/* MenuGen -- Data.h
 *
 * Copyright Stephen Fryatt, 2010
 *
 * Handle and track menu definition data.
 */


#ifndef _MENUGEN_DATA_H
#define _MENUGEN_DATA_H

#define MAX_TAG_LEN 32
#define MAX_TEMPLATE_NAME 16

int data_create_new_menu(char *tag, char *title);
int data_create_new_item(char *text);
int data_set_item_submenu(char *tag, int dbox);
int data_set_menu_title_indirection(int size);
int data_set_item_indirection(int size);
int data_set_item_writable(void);

#endif

