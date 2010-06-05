/* MenuGen -- Stack.c
 *
 * Copyright Stephen Fryatt, 2010
 *
 * Implement a simple integer stack API.
 */

#include <stdlib.h>

/* Local source headers. */

#include "stack.h"

static int stack_size = 0;
static int stack_ptr = -1;
static int *stack = NULL;

/**
 * Initialise the stack.
 *
 * \Param  size		The number of integers that the stack will hold.
 * \Return		0 if the initialise was successful; else 1.
 */

int stack_initialise(int size)
{
	stack_size = size;
	stack_ptr = -1;

	stack = (int *) malloc(sizeof(int) * stack_size);

	return (stack == NULL) ? 1 : 0;
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

