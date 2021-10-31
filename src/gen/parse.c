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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Local source headers. */

#include "parse.h"

#include "data.h"
#include "stack.h"

#define MAX_PARAM_LIST 10
#define MAX_PARAM_LEN 256

#define MAX_COMMAND_LEN 20

enum type {
	TYPE_NONE = 0,
	TYPE_MENU = 1,
	TYPE_ITEM = 2,
	TYPE_SUBMENU = 3,
	TYPE_WRITABLE = 4,
	TYPE_SPRITE = 5
};

struct command_def {
	char		command[MAX_COMMAND_LEN];
	char		params[MAX_PARAM_LIST];
	bool		menu;
	bool		item;
	bool		submenu;
	bool		writable;
	bool		sprite;
	enum type	new_type;
	bool		(*handler)(char params[][MAX_PARAM_LEN]);
};

static int parse_find_parameters(char params[][MAX_PARAM_LEN], char *line, char *types);

static bool parse_command_always(char params[][MAX_PARAM_LEN]);
static bool parse_command_colours_menu(char params[][MAX_PARAM_LEN]);
static bool parse_command_colours_item(char params[][MAX_PARAM_LEN]);
static bool parse_command_dbox(char params[][MAX_PARAM_LEN]);
static bool parse_command_dotted(char params[][MAX_PARAM_LEN]);
static bool parse_command_indirected_menu(char params[][MAX_PARAM_LEN]);
static bool parse_command_indirected_item(char params[][MAX_PARAM_LEN]);
static bool parse_command_item(char params[][MAX_PARAM_LEN]);
static bool parse_command_item_height(char params[][MAX_PARAM_LEN]);
static bool parse_command_item_gap(char params[][MAX_PARAM_LEN]);
static bool parse_command_menu(char params[][MAX_PARAM_LEN]);
static bool parse_command_reverse(char params[][MAX_PARAM_LEN]);
static bool parse_command_shaded(char params[][MAX_PARAM_LEN]);
static bool parse_command_submenu(char params[][MAX_PARAM_LEN]);
static bool parse_command_ticked(char params[][MAX_PARAM_LEN]);
static bool parse_command_validation(char params[][MAX_PARAM_LEN]);
static bool parse_command_warning(char params[][MAX_PARAM_LEN]);
static bool parse_command_writable(char params[][MAX_PARAM_LEN]);


/* Define the commands here, in terms of name, parameters, what commands they
 * must be subservient to, what groups they open, and what handlers they
 * use.
 */

#define COMMANDS 20

static const struct command_def command_list[] = {
	{"always",	"",	true,	true,	true,	false,	false,	TYPE_NONE,	parse_command_always},
	{"colours",	"IIII",	true,	false,	false,	false,	false,	TYPE_NONE,	parse_command_colours_menu},
	{"colours",	"II",	true,	true,	false,	false,	false,	TYPE_NONE,	parse_command_colours_item},
	{"d_box",	"I",	true,	true,	false,	false,	false,	TYPE_SUBMENU,	parse_command_dbox},
	{"dotted",	"",	true,	true,	false,	false,	false,	TYPE_NONE,	parse_command_dotted},
	{"half",	"",	true,	true,	false,	false,	true,	TYPE_NONE,	NULL},
	{"indirected",	"I",	true,	false,	false,	false,	false,	TYPE_NONE,	parse_command_indirected_menu},
	{"indirected",	"I",	true,	true,	false,	false,	false,	TYPE_NONE,	parse_command_indirected_item},
	{"item",	"S",	true,	false,	false,	false,	false,	TYPE_ITEM,	parse_command_item},
	{"item_gap",	"I",	true,	false,	false,	false,	false,	TYPE_NONE,	parse_command_item_gap},
	{"item_height",	"I",	true,	false,	false,	false,	false,	TYPE_NONE,	parse_command_item_height},
	{"menu",	"IS",	false,	false,	false,	false,	false,	TYPE_MENU,	parse_command_menu},
	{"reverse",	"",	true,	false,	false,	false,	false,	TYPE_NONE,	parse_command_reverse},
	{"shaded",	"",	true,	true,	false,	false,	false,	TYPE_NONE,	parse_command_shaded},
	{"sprite",	"",	true,	true,	false,	false,	false,	TYPE_SPRITE,	NULL},
	{"submenu",	"I",	true,	true,	false,	false,	false,	TYPE_SUBMENU,	parse_command_submenu},
	{"ticked",	"",	true,	true,	false,	false,	false,	TYPE_NONE,	parse_command_ticked},
	{"validation",	"S",	true,	true,	false,	true,	false,	TYPE_NONE,	parse_command_validation},
	{"warning",	"",	true,	true,	true,	false,	false,	TYPE_NONE,	parse_command_warning},
	{"writable",	"",	true,	true,	false,	false,	false,	TYPE_WRITABLE,	parse_command_writable}
};

/**
 * Process a file, reading bytes from the input and passing complete lines
 * to the parameter system.
 *
 * \Param  *filename		The file to process.
 * \Param  verbose		True if verbose output is to be written to STDOUT; else False.
 * \Return			True if the parsing completed successfully; else False.
 */

bool parse_process_file(char *filename, bool verbose)
{
	FILE	*f;
	bool	parse_error = false, fatal_error = false;
	int	c, last, len, pcount, i, cid;
	bool	comment = false, string = false;
	bool	menu = false, item = false, submenu = false, writable = false, sprite = false;
	int	line_number = 1;
	char	command[4096], params[MAX_PARAM_LIST][MAX_PARAM_LEN], types[64];

	last = '\0';
	len = 0;

	f = fopen(filename, "r");

	if (f != NULL) {
		while (!fatal_error && (c = fgetc(f)) != EOF) {
			if (c == '\n')
				line_number++;

			if (c == '*' && last == '/') {
				if (comment) {
					printf("Nested comments at line %d\n", line_number);
					parse_error = true;
				}

				comment = true;
				if (len > 0)
					len--;
			}

			if (c == '/' && last == '*') {
				if (!comment) {
					printf("No comment to close at line %d\n", line_number);
					parse_error = true;
				}

				comment = false;
				c = '\0';
			}

			if (c == '"')
				string = !string;

			if (!comment && ((c > 32) || (string && (c == 32)))) {
				if (c == '{' && !string) {
					command[len] = '\0';
					pcount = parse_find_parameters(params, command, types);

					if (pcount == 0) {
						printf("Error processing command '%s' at line %d\n", command, line_number);
						parse_error = true;
					}

					cid = -1;
					for (i = 0; (pcount > 0) && (i < COMMANDS); i++) {
						if (strcmp(command_list[i].command, params[0]) == 0 &&
								command_list[i].new_type != TYPE_NONE &&
								command_list[i].menu == menu &&
								command_list[i].item == item &&
								command_list[i].submenu == submenu &&
								command_list[i].writable == writable &&
								command_list[i].sprite == sprite) {
							cid = i;
							break;
						}
					}

					if (cid != -1) {
						if (strcmp(command_list[cid].params, types) == 0) {
							if (command_list[cid].handler != NULL)
								fatal_error = !command_list[cid].handler(params);
							if (fatal_error)
								printf("Internal error processing '%s' command at line %d\n", command_list[cid].command, line_number);
							else if (verbose)
								printf("Found command %s as section head at line %d\n", command_list[cid].command, line_number);
						} else {
							printf("Bad parameters to '%s' at line %d\n", command_list[cid].command, line_number);
							parse_error = true;
						}

						stack_push(command_list[cid].new_type);
						switch(command_list[cid].new_type) {
						case TYPE_MENU:
							menu = true;
							break;
						case TYPE_ITEM:
							item = true;
							break;
						case TYPE_SUBMENU:
							submenu = true;
							break;
						case TYPE_WRITABLE:
							writable = true;
							break;
						case TYPE_SPRITE:
							sprite = true;
							break;
						case TYPE_NONE:
							break;
						}
					} else {
						printf("Invalid command '%s' at line %d\n", command, line_number);
						parse_error = true;
					}
					len = 0;
				} else if (c == '}' && !string) {
					switch(stack_pop()) {
					case TYPE_MENU:
						menu = false;
						if (verbose)
							printf("Closing menu at line %d\n", line_number);
						break;
					case TYPE_ITEM:
						item = false;
						if (verbose)
							printf("Closing item at line %d\n", line_number);
						break;
					case TYPE_SUBMENU:
						submenu = false;
						if (verbose)
							printf("Closing submenu or d_box at line %d\n", line_number);
						break;
					case TYPE_WRITABLE:
						writable = false;
						if (verbose)
							printf("Closing writable at line %d\n", line_number);
						break;
					case TYPE_SPRITE:
						sprite = false;
						if (verbose)
							printf("Closing sprite at line %d\n", line_number);
						break;
					case TYPE_NONE:
						break;
					}
					len = 0;
				} else if (c == ';' && !string) {
					command[len] = '\0';
					pcount = parse_find_parameters(params, command, types);

					if (pcount == 0) {
						printf("Error processing command '%s' at line %d\n", command, line_number);
						parse_error = true;
					}

					cid = -1;
					for (i = 0; (pcount > 0) && (i < COMMANDS); i++) {
						if (strcmp(command_list[i].command, params[0]) == 0 &&
								command_list[i].menu == menu &&
								command_list[i].item == item &&
								command_list[i].submenu == submenu &&
								command_list[i].writable == writable &&
								command_list[i].sprite == sprite) {
							cid = i;
							break;
						}
					}

					if (cid != -1) {
						if (strcmp(command_list[cid].params, types) == 0) {
							if (command_list[cid].handler != NULL)
								fatal_error = !command_list[cid].handler(params);
							if (fatal_error)
								printf("Internal error processing '%s' command at line %d\n", command_list[cid].command, line_number);
							else if (verbose)
								printf("Found command %s standalone at line %d\n", command_list[cid].command, line_number);
						} else {
							printf("Bad parameters to '%s' at line %d\n", command_list[cid].command, line_number);
							parse_error = true;
						}
					} else {
						printf("Invalid command '%s' at line %d\n", command, line_number);
						parse_error = true;
					}
					len = 0;
				} else if (c != '\0') {
					command[len++] = c;
				}
			}

			last = c;
		}

		fclose(f);
	} else {
		printf("Bad source file '%s'\n", filename);
		fatal_error = true;
	}

	return (parse_error || fatal_error) ? false : true;
}

/**
 * Split the parameters from the command line passed in, returning them as
 * a set of strings and producing a list of types as a string in the form
 * "SI" for String-Integer.
 *
 * \param *params		An array of strings to take returned parameters
 * \param *line			The command line to parse
 * \param *types		A string to take the list of parameter types
 * \return			The number of parameters found including
 *				command name (so 0 == error)
 */

int parse_find_parameters(char params[][MAX_PARAM_LEN], char *line, char *types)
{
	bool	error = false;
	int	entries = 0;
	char	*copy, *tail, *right, *end;

	*types = '\0';

	copy = (char *) malloc(strlen(line) + 1);

	if (copy == NULL)
		return entries;

	strcpy(copy, line);
	end = copy + strlen(copy);

	tail = strchr(copy, '(');

	if (tail != NULL) {
		*tail++ = '\0';
		*--end = '\0'; /* Remove the trailing '('. */

		while (!error && tail < end && entries < 10) {
			right = strchr(tail, ',');
			if (right != NULL)
				*right = '\0';

			right = tail + strlen(tail) - 1;

			if (*tail == '"' && *right == '"') {
				tail++;
				*right = '\0';

				strcat(types, "S");
			} else if (*tail != '"' || *right != '"') {
				strcat(types, "I");
			} else {
				error = true;
			}

			strcpy(params[++entries], tail);

			tail = right + 2;
		}
	}

	strcpy(params[0], copy);
	entries++;

	free(copy);

	if (error)
		entries = 0;

	return entries;
}


/**
 * The various command handlers.
 */

static bool parse_command_always(char params[][MAX_PARAM_LEN])
{
	return data_set_item_when_shaded();
}

static bool parse_command_colours_menu(char params[][MAX_PARAM_LEN])
{
	return data_set_menu_colours(atoi(params[1]), atoi(params[2]), atoi(params[3]), atoi(params[4]));
}

static bool parse_command_colours_item(char params[][MAX_PARAM_LEN])
{
	return data_set_item_colours(atoi(params[1]), atoi(params[2]));
}

static bool parse_command_dbox(char params[][MAX_PARAM_LEN])
{
	return data_set_item_submenu(params[1], true);
}

static bool parse_command_dotted(char params[][MAX_PARAM_LEN])
{
	return data_set_item_dotted();
}

static bool parse_command_indirected_menu(char params[][MAX_PARAM_LEN])
{
	return data_set_menu_title_indirection(atoi(params[1]));
}

static bool parse_command_indirected_item(char params[][MAX_PARAM_LEN])
{
	return data_set_item_indirection(atoi(params[1]));
}

static bool parse_command_item(char params[][MAX_PARAM_LEN])
{
	return data_create_new_item(params[1]);
}

static bool parse_command_item_gap(char params[][MAX_PARAM_LEN])
{
	return data_set_menu_item_gap(atoi(params[1]));
}

static bool parse_command_item_height(char params[][MAX_PARAM_LEN])
{
	return data_set_menu_item_height(atoi(params[1]));
}

static bool parse_command_menu(char params[][MAX_PARAM_LEN])
{
	return data_create_new_menu(params[1], params[2]);
}

static bool parse_command_reverse(char params[][MAX_PARAM_LEN])
{
	return data_set_menu_reversed();
}

static bool parse_command_shaded(char params[][MAX_PARAM_LEN])
{
	return data_set_item_shaded();
}

static bool parse_command_submenu(char params[][MAX_PARAM_LEN])
{
	return data_set_item_submenu(params[1], false);
}

static bool parse_command_ticked(char params[][MAX_PARAM_LEN])
{
	return data_set_item_ticked();
}

static bool parse_command_validation(char params[][MAX_PARAM_LEN])
{
	return data_set_item_validation(params[1]);
}

static bool parse_command_warning(char params[][MAX_PARAM_LEN])
{
	return data_set_item_warning();
}

static bool parse_command_writable(char params[][MAX_PARAM_LEN])
{
	return data_set_item_writable();
}

