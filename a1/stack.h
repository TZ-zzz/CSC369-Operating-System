#include "defines.h"

/*************************************************
 * -------------------CSC369----------------------
 * ------------------A1 Stack--------------------- 
 * Pop functions: 
 *      If success, store the item into 
 *      the *item* pointer and return 0. 
 *      If failure, return ERROR CODE.
 * Push functions:
 *      If success, return 0.
 *      If failure, return ERROR CODE.
 * Initialize functions:
 *      Prepare all the data you need to start.
 *      If success, return 0.
 *      If failure, return ERROR CODE.
 * Destroy functions:
 *      Deallocated everything you malloced
 *      and clean up.
 *      If success, return 0.
 *      If failure, return ERROR CODE.
 * 
 * Find the ERROR CODE in defines.h
 *************************************************/

int stack_A_initialize();
int stack_A_pop(void** item);
int stack_A_push(void* item);
int stack_A_destroy();


int stack_B_initialize();
int stack_B_pop(void** item);
int stack_B_push(void* item);
int stack_B_destroy();


int stack_C_initialize();
int stack_C_pop(void** item);
int stack_C_push(void* item);
int stack_C_destroy();
