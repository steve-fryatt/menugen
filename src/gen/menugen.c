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

/* MenuGen
 *
 * Generate menu definition blocks for RISC OS in a cross-compilation
 * environment.
 *
 * Syntax: MenuGen <source> <output> [<options>]
 *
 * Options -d  - Embed dialogue box names into the output
 *         -m  - Embed menu names into the output
 *         -v  - Produce verbose output
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* We use types from this, but don't try to link to any subroutines! */

#include "oslib/wimp.h"

/* Local source headers. */

#include "data.h"
#include "parse.h"
#include "stack.h"


#define MAX_STACK_SIZE 100

int main(int argc, char *argv[])
{
	int	param;
	bool	verbose_output = false;
	bool	embed_dialogue_names = false;
	bool	embed_menu_names = false;
	bool	param_error = false;

	stack_initialise(MAX_STACK_SIZE);

	printf("MenuGen %s - %s\n", BUILD_VERSION, BUILD_DATE);
	printf("Copyright Stephen Fryatt, 2001-%s\n", BUILD_DATE + 7);

	if (argc < 3)
		param_error = true;

	if (!param_error) {
		for (param = 3; param < argc; param++) {
			if (strcmp(argv[param], "-d") == 0)
				embed_dialogue_names = true;
			else if (strcmp(argv[param], "-m") == 0)
				embed_menu_names = true;
			else if (strcmp(argv[param], "-v") == 0)
				verbose_output = true;
			else
				param_error = true;
		}
	}

	if (param_error) {
		printf("Usage: menugen <sourcefile> <output> [-d] [-m] [-v]\n");
		return 1;
	}

	printf("Starting to parse menu definition file...\n");
	if (!parse_process_file(argv[1], verbose_output)) {
		printf("Errors in source file: terminating.\n");
		return 1;
	}

	printf("Collating menu data...\n");
	data_collate_structures(embed_menu_names, embed_dialogue_names, verbose_output);

	if (verbose_output) {
		printf("Printing structure report...\n");
		data_print_structure_report();
	}

	printf("Writing menu file...\n");
	data_write_standard_menu_file(argv[2]);

	return 0;
}
