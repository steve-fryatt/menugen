/* MenuGen
 *
 * Generate menu definition blocks for RISC OS.
 *
 * (c) Stephen Fryatt, 1996-2010
 * Version 1.00 (7 May 2010)
 *
 * Syntax: MenuGen [<options>]
 *
 * Options -In <file>    - definition file
 *         -Out <file>   - menu file to generate
 *         -Verbose      - display details of parsing
 *         -Messagetrans - create file for MessageTrans_MakeMenus
 *         -Embed        - embed template names for dialogue boxes.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "data.h"
#include "stack.h"

#define MAX_PARAM_LEN 256
#define MAX_STACK_SIZE 100

#define TYPE_NONE     0
#define TYPE_MENU     1
#define TYPE_ITEM     2
#define TYPE_SUBMENU  3
#define TYPE_WRITABLE 4


typedef struct block_head {
	int	dialogues;
	int	indirected;
	int	validation;
};

struct command_def {
	char	command[MAX_COMMAND_LEN];
	char	params[MAX_PARAM_LEN];
	int	menu;
	int	item;
	int	submenu;
	int	writable;
	int	new_type;
	void	(*handler)(char params[][MAX_PARAM_LENGTH]);
}

void process_file(char *filename);
int find_parameters(char params[][MAX_PARAM_LEN], char *line, char *types);
void report_error(char *message);
void report_log(char *message);

static int line_number = 1;
static int error = 0;
static int verbose_output = 0;

#define COMMANDS 7

static const struct command_def command_list[] = {
	{"menu", "IS", 0, 0, 0, 0, TYPE_MENU, command_menu},
	{"item", "S", 1, 0, 0, 0, TYPE_ITEM, command_item},
	{"submenu", "I", 1, 1, 0, 0, TYPE_SUBMENU, command_submenu},
	{"d_box", "S", 1, 1, 0, 0, TYPE_SUBMENU, command_dbox},
	{"writable", "", 1, 1, 0, 0, TYPE_WRITABLE, command_writable},
	{"indirected", 1, 1, 0, 0, TYPE_NONE, command_indirected_item},
	{"indirected", 1, 0, 0, 0, TYPE_NONE, command_indirected_menu}
};



int main(void)
{
	char params[10][MAX_PARAM_LEN], types[64];
	int found, i;

	stack_initialise(MAX_STACK_SIZE);

	found = find_parameters(params, "command(\"string\",\"string2\",765,877)", types);

	printf("Returned types: %s\n", types);

	for (i=0; i<found; i++) {
		printf("Param %d: '%s'\n", i, params[i]);
	}
}



/**
 * Process a file, reading bytes from the input and passing complete lines
 * to the parameter system.
 *
 * \Param  *filename		The file to process.
 */

void process_file(char *filename)
{
	FILE	*f;
	int	c, last, len, pcount, i, cid;
	int	comment, string, menu, item, submenu, writable;
	char	command[4096], params[10][MAX_PARAM_LEN], types[64];


	comment = 0;
	string = 0;
	menu = 0;
	item = 0;
	submenu = 0;
	writable = 0;

	last = '\0';
	len = 0;

	f = fopen(filename, "r");

	if (f != NULL) {
		while ((c = fgetc(f)) != EOF) {
			if (c == '\n')
				line_number++;

			if (c == '*' && last == '/') {
				if (comment)
					report_error("Nested comments");

				comment = 1;
				if (len > 0)
					len--;

				report_log(" <comment>");
			}

			if (c == '/' && last == '*') {
				if (!comment)
					report_error("No comment to close");

				comment = 0;
				c = '\0';
			}

			if (c == '"')
				string = !string;

			if (!comment && ((c > 32) || (string && (c == 32)))) {
				if (c == '{' && !string) {
					command[len] = '\0';
					pcount = find_parameters(params, command, types);

					cid = -1
					for (i = 0; i < COMMANDS; i++) {
						if (strcmp(command_list[i].command, params[0]) == 1 &&
								command_list[i],new_type != TYPE_NONE &&
								command_list[i].menu == menu &&
								command_list[i].item == item &&
								command_list[i].submenu == submenu &&
								command_list[i].writable == writable) {
							cid = i;
							break;
						}

						if (cid != -1) {
							if (strcmp(command_list[i].types, types) == 0) {
								command_list[i].handler(params);
								stack_push(command_list[i].new_type);
								switch(command_list[i].new_type) {
								case TYPE_MENU:
									menu = 1;
									break;
								case TYPE_ITEM:
									item = 0;
									break;
								case TYPE_SUBMENU:
									submenu = 0;
									break;
								case TYPE_WRITABLE:
									writable = 0;
									break;
								}

							} else {
								report_error("Bad parameters");
							}
						} else {
							report_error("Invalid command");
						}
					}

					command[len] = '\0';
					pcount = find_parameters(params, command, types);

					if (strcmp(params[0], "menu") == 0) {
						if (strcmp(types, "IS") == 0) {
							if (menu) {
								report_error("menus can not be nested");
							} else {
								menu = 1;
								stack_push(TYPE_MENU);
								data_create_new_menu(params[1], params[2]);
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "item") == 0) {
						if (strcmp(types, "S") == 0) {
							if (!menu) {
								report_error("items must be within a menu");
							} else {
								if (item) {
									report_error("items can not be nested");
								} else {
									item = 1;
									stack_push(TYPE_ITEM);
									data_create_new_item(params[1]);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "sumbenu") == 0) {
						if (strcmp(types, "I") == 0) {
							if (!item) {
								report_error("submenu must be within an item");
							} else {
								if (submenu) {
									report_error("submenu can not be nested");
								} else {
									submenu = 1;
									stack_push(TYPE_SUBMENU);
									data_set_item_submenu(params[1], 0);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "d_box") == 0) {
						if (strcmp(types, "S") == 0) {
							if (!item) {
								report_error("d_box must be within an item");
							} else {
								if (submenu) {
									report_error("submenu can not be nested");
								} else {
									submenu = 1;
									stack_push(TYPE_SUBMENU);
									data_set_item_submenu(params[1], 1);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "writable") == 0) {
						if (strcmp(types, "") == 0) {
							if (!item) {
								report_error("writable must be within an item");
							} else {
								if (writable) {
									report_error("writable can not be nested");
								} else {
									writable = 1;
									stack_push(TYPE_WRITABLE);
									data_set_item_writable();
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else {
						report_error(strcat(params[0], "cannot take statements"));
					}
				} else if (c == '}' && !string) {
					switch(stack_pop()) {
					case TYPE_MENU:
						menu = 0;
						break;
					case TYPE_ITEM:
						item = 0;
						break;
					case TYPE_SUBMENU:
						submenu = 0;
						break;
					case TYPE_WRITABLE:
						writable = 0;
						break;
					}
					len = 0;
				} else if (c == ';' && !string) {
					command[len] = '\0';
					pcount = find_parameters(params, command, types);

					if (strcmp(params[0], "menu") == 0) {
						if (strcmp(types, "IS") == 0) {
							if (menu) {
								report_error("menus can not be nested");
							} else {
								data_create_new_menu(params[1], params[2]);
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "item") == 0) {
						if (strcmp(types, "S") == 0) {
							if (!menu) {
								report_error("items must be within a menu");
							} else {
								if (item) {
									report_error("items can not be nested");
								} else {
									data_create_new_item(params[1]);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "sumbenu") == 0) {
						if (strcmp(types, "I") == 0) {
							if (!item) {
								report_error("submenu must be within an item");
							} else {
								if (submenu) {
									report_error("submenu can not be nested");
								} else {
									data_set_item_submenu(params[1], 0);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "d_box") == 0) {
						if (strcmp(types, "S") == 0) {
							if (!item) {
								report_error("d_box must be within an item");
							} else {
								if (submenu) {
									report_error("submenu can not be nested");
								} else {
									data_set_item_submenu(params[1], 1);
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "writable") == 0) {
						if (strcmp(types, "") == 0) {
							if (!item) {
								report_error("writable must be within an item");
							} else {
								if (writable) {
									report_error("writable can not be nested");
								} else {
									data_set_item_writable();
								}
							}
						} else {
							report_error("Bad parameters");
						}
					} else if (strcmp(params[0], "indirected") == 0) {
						if (strcmp(types, "I") == 0) {

						} else {
							report_error
						}
					}
				} else if (c != '\0') {
					command[len++] = c;
				}
			}
		}

		fclose(f);
	} else {
		report_error("Bad source filename");
	}
}

void command_menu(char params[][MAX_PARAM_LEN])
{
	data_create_new_menu(params[1], params[2]);
}

void command_item(char params[][MAX_PARAM_LEN])
{
	data_create_new_item(params[1]);
}

void command_submenu(char params[][MAX_PARAM_LEN])
{
	data_set_item_submenu(params[1], 0);
}

void command_dbox(char params[][MAX_PARAM_LEN])
{
	data_set_item_submenu(params[1], 1);
}

void command_writable(char params[][MAX_PARAM_LEN])
{
	data_set_item_writable();
}


/**
 * Split the parameters from the command line passed in, returning them as
 * a set of strings and producing a list of types as a string in the form
 * "SI" for String-Integer.
 *
 * \Param  *params		An array of strings to take returned parameters
 * \Param  *line		The command line to parse
 * \Param  *types		A string to take the list of parameter types
 * \Return			The number of parameters found including
 *				command name (so 0 == error)
 */

int find_parameters(char params[][MAX_PARAM_LEN], char *line, char *types)
{
	int	entries;
	char	*copy, *tail, *right, *end;

	*types = '\0';
	entries = 0;
	error = 0;

	copy = (char *) malloc(strlen(line + 1));

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
				report_error("Bad string");
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
 * Output a message to STDOUT.
 *
 * Output lines are not automatically terminated with a \n, allowing log
 * entries to be concatanated if required.
 *
 * \Param  *message		The message to display.
 */

void report_log(char *message)
{
	if (verbose_output)
		printf("%s", message);
}


/**
 * Report an error to STDOUT.
 *
 * \Param  *message		The message to display.
 */

void report_error(char *message)
{
	printf("Error: %s at line %d\n", message, line_number);
	error = 1;
}


