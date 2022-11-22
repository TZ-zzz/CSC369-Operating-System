#include "pagetable_generic.h"
#include <stdlib.h>

struct frame* head;
struct frame* tail;
struct frame* list;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int lru_evict(void)
{
	struct frame* temp = tail;
	tail = tail->prev;
	tail->next = NULL;
	temp->prev = NULL;
	temp->next = NULL;
	return temp->frame;
}

/* This function is called on each access to a page to update any information
 * needed by the LRU algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(int frame)
{
	struct frame* f = &list[frame];
	if (f == head) {
		return;
	}
	if (f == tail) {
		tail = tail->prev;
		tail->next = NULL;
	} else {
		if (f->prev != NULL) {
			f->prev->next = f->next;
		}
		if (f->next != NULL) {
			f->next->prev = f->prev;
		}
	}
	f->next = head;
	f->prev = NULL;
	head->prev = f;
	head = f;
}

/* Initialize any data structures needed for this replacement algorithm. */
void lru_init(void)
{
	// init a list with m entries which takes O(m) time
	list = malloc(sizeof(struct frame) * memsize);
	for (size_t i = 0; i < memsize; i++) {
		list[i].frame = i;
		list[i].next = NULL;
		list[i].prev = NULL;
	}
	head = &list[0];
	tail = &list[0];
}

/* Cleanup any data structures created in lru_init(). */
void lru_cleanup(void)
{
	free(list);
}
