/* Copyright 1996-2012, Stephen Fryatt
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
 * distributed on an "AS IS" basis,
 *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */


#ifndef _MENUGEN_DATA_H
#define _MENUGEN_DATA_H

#define MAX_TAG_LEN 32
#define MAX_TEMPLATE_NAME 16

int data_collate_structures(int embed_dbox, int verbose);
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

