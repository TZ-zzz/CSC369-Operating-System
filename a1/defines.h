#include <stdlib.h>

/*************************************************
 * -------------------CSC369----------------------
 * ------------------A1 Defines--------------------- 
 *************************************************/

#define malloc malloc_a1
#define free free_a1
#define calloc(n,s) malloc(n*s)
#define realloc realloc_a1

/* ERROR CODE */
#define ERR_NO_MEM          -1      // The maximum capacity is reached
#define ERR_INVALID_ARG     -2      // Invalid pointer is given when dequeue/pop
#define ERR_NO_ITEM         -3      // The stack/queue is empty
#define ERR_NOT_INITALIZED  -4      // Initialization function is not called yet
#define ERR_INITIALIZED     -5      // Try to initialize when already initialized

#define PART_B_MAX_SIZE 1024

void * malloc_a1(size_t size); 
void free_a1(void * ptr);
void * realloc_a1(void * ptr, size_t new_size);
