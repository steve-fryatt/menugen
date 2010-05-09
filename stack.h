/* MenuGen -- Stack.h
 *
 * Copyright Stephen Fryatt, 2010
 *
 * Implement a simple integer stack API.
 */

#define STACK_EMPTY -1

int stack_initialise(int size);
void stack_terminate(void);
void stack_push(int value);
int stack_pop(void);
int stack_top(void);

