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


#ifndef MENUTEST_FILE_H
#define MENUTEST_FILE_H

#include <stdint.h>

/**
 * Load a file into memory, returning a pointer to a malloc()-claimed block
 * and optionally the size of the data.
 *
 * \param *filename	Pointer to the name of the file to load.
 * \param *length	Pointer to a variable to take the block length, or NULL.
 * \return		Pointer to the loaded block, on NULL on failure.
 */

int8_t *file_load(char *filename, size_t *length);

#endif

