/* Copyright 1996-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of MenuGen:
 *
 *   http://www.stevefryatt.org.uk/software/
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

#include <stdbool.h>
#include <stdlib.h>

/* Local source headers. */

#include "stack.h"

static int stack_size = 0;
static int stack_ptr = -1;
static int *stack = NULL;

/**
 * Initialise the stack.
 *
 * \param size		The number of integers that the stack will hold.
 * \return		True if the initialise was successful; else False.
 */

bool stack_initialise(int size)
{
	stack_size = size;
	stack_ptr = -1;

	stack = (int *) malloc(sizeof(int) * stack_size);

	return (stack == NULL) ? false : true;
}

/**
 * Terminate the stack and free the resources it uses.
 */

void stack_terminate(void)
{
	if (stack != NULL)
		free(stack);

	stack = NULL;
	stack_ptr = -1;
	stack_size = 0;
}

/**
 * Push a value on to the stack.
 *
 * \Param  value	The value to push on to the stack.
 */

void stack_push(int value)
{
	if ((stack != NULL) && (stack_ptr < (stack_size-1)))
		stack[++stack_ptr] = value;
}

/**
 * Pop a value off the stack.
 *
 * \Return		The value from the top of the stack (or -1 if the
 *			stack is empty).
 */

int stack_pop(void)
{
	if ((stack != NULL) && (stack_ptr > -1))
		return stack[stack_ptr--];
	else
		return STACK_EMPTY;
}

/**
 * Return the value from the top of the stack, leaving it in situ.
 *
 * \Return		The value from the top of the stack (or -1 if the
 *			stack is empty).
 */

int stack_top(void)
{
	if ((stack != NULL) && (stack_ptr > -1))
		return stack[stack_ptr];
	else
		return STACK_EMPTY;
}

