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


#ifndef MENUGEN_DATA_H
#define MENUGEN_DATA_H

#include <stdbool.h>

#define MAX_TAG_LEN 32
#define MAX_TEMPLATE_NAME 16

bool data_collate_structures(bool embed_tag, bool embed_dbox, bool verbose);
void data_print_structure_report(void);
bool data_write_standard_menu_file(char *filename);

bool data_create_new_menu(char *tag, char *title);
bool data_create_new_item(char *text);
bool data_set_item_submenu(char *tag, bool dbox);
bool data_set_menu_title_indirection(int size);
bool data_set_item_indirection(int size);
bool data_set_item_writable(void);
bool data_set_item_validation(char *validation);
bool data_set_menu_colours(int title_fg, int title_bg, int work_fg, int work_bg);
bool data_set_item_colours(int icon_fg, int icon_bg);
bool data_set_menu_reversed(void);
bool data_set_menu_item_height(int height);
bool data_set_menu_item_gap(int gap);
bool data_set_item_ticked(void);
bool data_set_item_dotted(void);
bool data_set_item_warning(void);
bool data_set_item_when_shaded(void);
bool data_set_item_shaded(void);

#endif

