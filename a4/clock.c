#include "pagetable_generic.h"

int clock_hand;

/* Page to evict is chosen using the CLOCK algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int clock_evict(void)
{
	while(get_referenced(coremap[clock_hand].pte) == 1) {
		set_referenced(coremap[clock_hand].pte, 0);
		clock_hand = (clock_hand + 1) % memsize;
	}
	int frame = clock_hand;
	clock_hand = (clock_hand + 1) % memsize;
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the CLOCK algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(int frame)
{
	set_referenced(coremap[frame].pte, 1);
}

/* Initialize any data structures needed for this replacement algorithm. */
void clock_init(void)
{
	clock_hand = 0;
}

/* Cleanup any data structures created in clock_init(). */
void clock_cleanup(void)
{

}
