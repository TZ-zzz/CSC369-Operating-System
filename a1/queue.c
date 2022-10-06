#include "queue.h"

/*************************************************
 * Implement the following functions.
 * DO NOT add any global (static) variables,
 * Except in Part C.
 * You can add help functions as you need.
 ************************************************/


/*************************************************
 * ------------------Part A:---------------------- 
 * In this part, you will implement
 * a  dynamically allocated queue with
 * a linked list. 
 * DO NOT add any global (static) variables.
 *************************************************/

int queue_A_initialized = 0;

typedef struct queue_A_node{

/* Add code BEGIN */
    struct queue_A_node * next;

/* Add code END */

    void* item;

} 
queue_A_node_t;

queue_A_node_t * queue_A_root = NULL;
queue_A_node_t * queue_A_tail = NULL;

/* Add code BEGIN */
// Help unctions only!



/* Add code END */

int queue_A_initialize(){

/* Add code BEGIN */
    if (queue_A_initialized != 0){
        return ERR_INITIALIZED;
    }
    queue_A_root = NULL;
    queue_A_initialized = 1;

/* Add code END */

    return 0;

}

int queue_A_enq(void* item){

/* Add code BEGIN */
    if (queue_A_initialized == 0) {
        return ERR_NOT_INITALIZED;
    }
    queue_A_node_t *new = malloc(sizeof(queue_A_node_t));
    new->next = NULL;
    new->item = item;
    if (queue_A_root == NULL){
        queue_A_root = queue_A_tail = new;
    }
    else {
        queue_A_tail->next = new;
        queue_A_tail = new;
    }

/* Add code END */

    return 0;

}

int queue_A_deq(void** item){

/* Add code BEGIN */
    if (queue_A_initialized == 0) {
        return ERR_NOT_INITALIZED;
    }
    if (queue_A_root == NULL){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    *item = queue_A_root->item;
    queue_A_node_t * curr_node = queue_A_root;
    if (queue_A_root->next != NULL){
        queue_A_root = queue_A_root->next;
    }
    else{
        queue_A_root = NULL;
        queue_A_tail = NULL;
    }
    free(curr_node);

/* Add code END */

    return 0;

}

int queue_A_destroy(){

/* Add code BEGIN */
    if (queue_A_initialized == 0) {
        return ERR_NOT_INITALIZED;
    }
    queue_A_tail = NULL;
    queue_A_node_t* curr_node;
    while (queue_A_root != NULL){
        curr_node = queue_A_root;
        queue_A_root = queue_A_root->next;
        free(curr_node);
    }
    queue_A_initialized = 0;

/* Add code END */

    return 0;

}

/* END of Part A */

/*************************************************
 * ------------------Part B:---------------------- 
 * In this part, you will implement a 
 * static queue with a max capacity using
 * the RING BUFFER introduced in the handout. 
 * DO NOT add any global (static) variables.
 * Rename the given variables to match your 
 * coding style. 
 *************************************************/

int queue_B_initialized = 0;

void * queue_B_items[PART_B_MAX_SIZE];

unsigned long queue_B_var_1 = 0, queue_B_var_2 = 0, queue_B_var_3 = 0;

/* Edit macros below for your convience BEGIN */
#define queue_B_item_count queue_B_var_1 
#define queue_B_head queue_B_var_2 
#define queue_B_tail queue_B_var_3
/* Edit macros END */ 


/* Add code BEGIN */
// Help unctions only!



/* Add code END */

int queue_B_initialize(){

/* Add code BEGIN */
    if (queue_B_initialized != 0){
        return ERR_INITIALIZED;
    }
    queue_B_initialized = 1;

/* Add code END */

    return 0;

}

int queue_B_enq(void* item){

/* Add code BEGIN */
    if (queue_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (queue_B_item_count >= PART_B_MAX_SIZE){
        return ERR_NO_MEM;
    }
    queue_B_items[queue_B_tail] = item;
    queue_B_tail += 1;
    if (queue_B_tail == PART_B_MAX_SIZE){
        queue_B_tail = 0;
    }
    queue_B_item_count += 1;

/* Add code END */

    return 0;

}

int queue_B_deq(void** item){

/* Add code BEGIN */
    if (queue_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (queue_B_item_count == 0){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    *item = queue_B_items[queue_B_head];
    queue_B_head += 1;
    if (queue_B_head == PART_B_MAX_SIZE){
        queue_B_head = 0;
    }
    queue_B_item_count -= 1;

/* Add code END */

    return 0;

}

int queue_B_destroy(){

/* Add code BEGIN */
    if (queue_B_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    queue_B_item_count =  queue_B_head = queue_B_tail = 0;
    queue_B_initialized = 0;

/* Add code END */

    return 0;

}

/* END of Part B */

/*************************************************
 * ------------------Part C:---------------------- 
 * In this part, you will implement a
 *  dynamically allocated queue. The queue 
 * should support any large number of items.
 * 
 * You are free to call malloc as you need.
 * However, unlike Part A where each enqueue
 * needs a malloc, you need to follow
 * the rules below WHEN the number of
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
 * even in the process of dequeue. 
 * 
 * You MAY add any necessary global (static)
 * variables in this part, as well as
 * structs/types and help functions. 
 *************************************************/

int queue_C_initialized = 0;

/* Add code BEGIN */
typedef struct Dynamic_Queue{
    void** array;
    size_t size;
    size_t item_count;
    size_t head;
    size_t tail;
}queue_C;

queue_C queue_C_items;

void queue_C_shrink(){

    for (int i = 0; i < queue_C_items.item_count; i ++){
        queue_C_items.array[i] = queue_C_items.array[i + queue_C_items.head];
    }
    if (queue_C_items.item_count < 1000){
        queue_C_items.array = realloc(queue_C_items.array, sizeof(void *) * 1000);
        queue_C_items.size = 1000;
    }
    else {
        queue_C_items.size = queue_C_items.item_count + 100;
        queue_C_items.array = realloc(queue_C_items.array, sizeof(void *) * queue_C_items.size);
    }
    queue_C_items.head = 0;
    queue_C_items.tail = queue_C_items.item_count;
    
}

/* Add code END */

int queue_C_initialize(){

/* Add code BEGIN */
    if (queue_C_initialized != 0){
        return ERR_INITIALIZED;
    }
    queue_C_items.array = malloc(sizeof(void *) * 1000);
    queue_C_items.head = queue_C_items.tail = 0;
    queue_C_items.item_count = 0;
    queue_C_items.size = 1000;
    queue_C_initialized = 1;

/* Add code END */

    return 0;

}

int queue_C_enq(void* item){

/* Add code BEGIN */
    if (queue_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (queue_C_items.tail == queue_C_items.size && (queue_C_items.item_count * 1.0) / ((queue_C_items.size + 100) * 1.0) > 0.82){
        queue_C_items.size += 100;
        queue_C_items.array = realloc(queue_C_items.array, sizeof(void *) * (queue_C_items.size));
    }
    else if (queue_C_items.tail == queue_C_items.size && queue_C_items.size >= 1000){
        queue_C_shrink();
    }
    queue_C_items.array[queue_C_items.tail] = item;
    queue_C_items.tail += 1;
    queue_C_items.item_count += 1;

/* Add code END */

    return 0;

}

int queue_C_deq(void** item){

/* Add code BEGIN */
    if (queue_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    if (queue_C_items.item_count== 0){
        return ERR_NO_ITEM;
    }
    if (item == NULL){
        return ERR_INVALID_ARG;
    }
    * item = queue_C_items.array[queue_C_items.head];
    queue_C_items.head += 1;
    queue_C_items.item_count -= 1;
    
    if (queue_C_items.size >= 1000 && (queue_C_items.item_count * 1.0) / (queue_C_items.size * 1.0) < 0.82){
        queue_C_shrink();
    }

/* Add code END */

    return 0;

}

int queue_C_destroy(){

/* Add code BEGIN */
    if (queue_C_initialized == 0){
        return ERR_NOT_INITALIZED;
    }
    free(queue_C_items.array);
    queue_C_initialized = 0;

/* Add code END */

    return 0;

}

/* End of Part C */
