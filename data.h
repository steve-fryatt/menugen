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

int data_collate_structures(int verbose);
void data_print_structure_report(void);
int data_write_standard_menu_file(char *filename);

int data_create_new_menu(char *tag, char *title);
int data_create_new_item(char *text);
int data_set_item_submenu(char *tag, int dbox);
int data_set_menu_title_indirection(int size);
int data_set_item_indirection(int size);
int data_set_item_writable(void);
int data_set_item_validation(char *validation);
int data_set_menu_colours(int title_fg, int title_bg, int work_fg, int work_bg);
int data_set_item_colours(int icon_fg, int icon_bg);
int data_set_menu_reversed(void);
int data_set_menu_item_height(int height);
int data_set_menu_item_gap(int gap);
int data_set_item_ticked(void);
int data_set_item_dotted(void);
int data_set_item_warning(void);
int data_set_item_when_shaded(void);
int data_set_item_shaded(void);

#endif

