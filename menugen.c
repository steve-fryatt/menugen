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
#include "parse.h"
#include "stack.h"


#define MAX_STACK_SIZE 100

static int verbose_output = 1;


int main(int argc, char *argv[])
{
	stack_initialise(MAX_STACK_SIZE);

	printf("MenuGen 2.00 - %s\n", "16-May-2010");
	printf("Copyright Stephen Fryatt, 2010\n");

	if (argc != 3) {
		printf("Usage: menugen <sourcefile> <output>\n");
		return 1;
	}

	printf("Starting to parse menu definition file...\n");
	parse_process_file(argv[1], verbose_output);

	printf("Collating menu data...\n");
	data_collate_structures(verbose_output);

	return 0;
}
