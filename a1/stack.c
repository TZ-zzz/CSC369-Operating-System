#include "stack.h"

/************************
 * Implement the following functions.
 * DO NOT add any global (static) variables,
 * Except in Part C.
 * You can add help functions as you need.
 * **********************/

/*************************************************
 * ------------------Part A:---------------------- 
 * In this part, you will implement
 * a  dynamically allocated stack with
 * a linked list. 
 * DO NOT add any global (static) variables.
 *************************************************/

int stack_A_initialized = 0;

typedef struct stack_A_node{

/* Add code BEGIN */
    struct stack_A_node *next;

/* Add code END */

    void* item;

} 
stack_A_node_t;

stack_A_node_t * stack_A_root = NULL;
stack_A_node_t * stack_A_tail = NULL;

/* Add code BEGIN */
// Help unctions only!



/* Add code END */

int stack_A_initialize(){

/* Add code BEGIN */
    if (stack_A_initialized != 0) {
        return ERR_INITIALIZED;
    }
    stack_A_root = NULL;
    stack_A_initialized = 1;

/* Add code END */

    return 0;

}

int stack_A_pop(void** item){

/* Add code BEGIN */
    if (stack_A_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (stack_A_root == NULL){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    *item = stack_A_root->item;
    stack_A_node_t* curr_node = stack_A_root;
    stack_A_root = stack_A_root->next;
    free(curr_node);

/* Add code END */

    return 0;

}

int stack_A_push(void* item){

/* Add code BEGIN */
    if (stack_A_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    stack_A_tail = malloc(sizeof(stack_A_node_t));
    stack_A_tail->item = item;
    if (stack_A_root == NULL){
        stack_A_tail->next = NULL;
    }
    else{
        stack_A_tail->next = stack_A_root;
    }
    stack_A_root = stack_A_tail;
    stack_A_tail = NULL;

/* Add code END */

    return 0;

}

int stack_A_destroy(){

/* Add code BEGIN */
    if (stack_A_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    stack_A_node_t* curr_node;
    
    while (stack_A_root != NULL){
        curr_node = stack_A_root;
        stack_A_root = stack_A_root->next;
        free(curr_node);
    }
    stack_A_initialized = 0;

/* Add code END */

    return 0;

}

/* END of Part A */

/*************************************************
 * ------------------Part B:---------------------- 
 * In this part, you will implement a 
 * static stack with a max capacity using
 * the RING BUFFER introduced in the handout. 
 * DO NOT add any global (static) variables.
 * Rename the given variables to match your 
 * coding style. 
 *************************************************/

int stack_B_initialized = 0;

void * stack_B_items[PART_B_MAX_SIZE];

unsigned long stack_B_var_1 = 0, stack_B_var_2 = 0, stack_B_var_3 = 0;

/* Edit macros below for your convience BEGIN */
#define item_count stack_B_var_1
#define top stack_B_var_2
#define bottom stack_B_var_3
/* Edit macros END */ 


/* Add code BEGIN */
// Help unctions only!



/* Add code END */

int stack_B_initialize(){

/* Add code BEGIN */
    if (stack_B_initialized != 0){
        return ERR_INITIALIZED;
    }
    stack_B_initialized = 1;

/* Add code END */

    return 0;

}

int stack_B_pop(void** item){

/* Add code BEGIN */
    if (stack_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (item_count == 0){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    *item = stack_B_items[item_count - 1];
    item_count -= 1;

/* Add code END */

    return 0;

}

int stack_B_push(void* item){

/* Add code BEGIN */
    if (stack_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (item_count >= PART_B_MAX_SIZE){
        return ERR_NO_MEM;
    }
    stack_B_items[item_count] = item;
    item_count += 1;

/* Add code END */

    return 0;

}

int stack_B_destroy(){

/* Add code BEGIN */
    if (stack_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    item_count = 0;
    stack_B_initialized = 0;

/* Add code END */

    return 0;

}

/* END of Part B */

/*************************************************
 * ------------------Part C:---------------------- 
 * In this part, you will implement a
 *  dynamically allocated stack. The stack 
 * should support any large number of items.
 * 
 * You are free to call malloc as you need.
 * However, unlike Part A where each push
 * needs a malloc, you need to follow
 * the rules below to minimize the use of 
 * the malloc WHEN the number of
 * elements reaches 1000:
 * 1.   the number of malloc calls shall not
 *      exceed 1 per 100 elements added
 * 2.   the overall space efficiency 
 *      (total bytes of the items stored/
 *      total bytes malloced) shall be higher
 *      than 80% 
 * You are also required to 
 * recycle all the space freed. That is,
 * the above rules shall always be kept
 * even in the process of pop. 
 * 
 * You MAY add any necessary global (static)
 * variables in this part, as well as
 * structs/types and help functions. 
 *************************************************/

int stack_C_initialized = 0;

/* Add code BEGIN */
typedef struct Dynamic_Stack {
    void ** array;
    size_t item_count;
    size_t size;
}stack_C;

stack_C stack_C_items;
/* Add code END */

int stack_C_initialize(){

/* Add code BEGIN */
    if (stack_C_initialized != 0){
        return ERR_INITIALIZED;
    }
    stack_C_items.size = 1000;
    stack_C_items.item_count = 0;
    stack_C_items.array = malloc(sizeof(void *) * 1000);
    stack_C_initialized = 1;

/* Add code END */

    return 0;

}

int stack_C_pop(void** item){

/* Add code BEGIN */
    if (stack_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (stack_C_items.item_count== 0){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    * item = stack_C_items.array[stack_C_items.item_count - 1];
    stack_C_items.item_count -= 1;
    
    if (stack_C_items.size > 1000 && (stack_C_items.item_count * 1.0) / (stack_C_items.size * 1.0) < 0.82){
        if (stack_C_items.item_count <= 1000){
            stack_C_items.array = realloc(stack_C_items.array, sizeof(void *) * 1000);
            stack_C_items.size = 1000;
        }
        else {
            stack_C_items.size = stack_C_items.item_count + 100;
            stack_C_items.array = realloc(stack_C_items.array, sizeof(void *) * stack_C_items.size);
        }
    }


/* Add code END */

    return 0;

}

int stack_C_push(void* item){

/* Add code BEGIN */
    if (stack_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (stack_C_items.item_count == stack_C_items.size){
        stack_C_items.size += 100;
        stack_C_items.array = realloc(stack_C_items.array, sizeof(void *) * (stack_C_items.size));
        
    }
    stack_C_items.array[stack_C_items.item_count] = item;
    stack_C_items.item_count += 1;

/* Add code END */

    return 0;

}

int stack_C_destroy(){

/* Add code BEGIN */
    if (stack_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    free(stack_C_items.array);
    stack_C_initialized = 0;

/* Add code END */

    return 0;

}

/* End of Part C */
