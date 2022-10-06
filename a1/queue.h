#include "defines.h"

/*************************************************
 * -------------------CSC369----------------------
 * ------------------A1 Stack--------------------- 
 * Dequeue functions: 
 *      If success, store the item into 
 *      the *item* pointer and return 0. 
 *      If failure, return ERROR CODE.
 * Enqueue functions:
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

int queue_A_initialize();
int queue_A_enq(void* item);
int queue_A_deq(void** item);
int queue_A_destroy();


int queue_B_initialize();
int queue_B_enq(void* item);
int queue_B_deq(void** item);
int queue_B_destroy();


int queue_C_initialize();
int queue_C_enq(void* item);
int queue_C_deq(void** item);
int queue_C_destroy();
