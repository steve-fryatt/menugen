/* Copyright 2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of MenuTest:
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
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


/* Local source headers. */

#include "file.h"


/**
 * Load a file into memory, returning a pointer to a malloc()-claimed block
 * and optionally the size of the data.
 *
 * \param *filename	Pointer to the name of the file to load.
 * \param *length	Pointer to a variable to take the block length, or NULL.
 * \return		Pointer to the loaded block, on NULL on failure.
 */

int8_t *file_load(char *filename, size_t *length)
{
	FILE	*file;
	long	len;
	int8_t	*data;

	/* Zero the returned file length. */

	if (length != NULL)
		*length = 0;

	/* If there's no filename, exit. */

	if (filename == NULL)
		return NULL;

	/* Open the file, then find its length. */

	file = fopen(filename, "rb");
	if (file == NULL)
		return NULL;

	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, SEEK_SET);

	/* Claim the required memory, then load the file. */

	data = malloc(len);
	if (data != NULL && (fread(data, sizeof(int8_t), len, file) * sizeof(int8_t)) != len) {
		free(data);
		data = NULL;
	} else if (length != NULL) {
		*length = len;
	}

	/* Close the file and exit. */

	fclose(file);

	return data;
}

