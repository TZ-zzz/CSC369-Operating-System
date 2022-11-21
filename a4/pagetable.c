/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Andrew Peterson, Karen Reid, Alexey Khrabrov
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019, 2021 Karen Reid
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pagetable_generic.h"
#include "pagetable.h"
#include "swap.h"


// Counters for various events.
// Your code must increment these when the related events occur.
size_t hit_count = 0;
size_t miss_count = 0;
size_t ref_count = 0;
size_t evict_clean_count = 0;
size_t evict_dirty_count = 0;

pd_entry_t pd[PT_SIZE]; 

/*
 * Allocates a frame to be used for the virtual page represented by p.
 * If all frames are in use, calls the replacement algorithm's evict_func to
 * select a victim frame. Writes victim to swap if needed, and updates
 * page table entry for victim to indicate that virtual page is no longer in
 * (simulated) physical memory.
 *
 * Counters for evictions should be updated appropriately in this function.
 */
static int allocate_frame(pt_entry_t *pte)
{
	int frame = -1;
	for (size_t i = 0; i < memsize; ++i) {
		if (!coremap[i].in_use) {
			frame = i;
			break;
		}
	}

	if (frame == -1) { // Didn't find a free page.
		// Call replacement algorithm's evict function to select victim
		frame = evict_func();
		assert(frame != -1);

		// All frames were in use, so victim frame must hold some page
		// Write victim page to swap, if needed, and update page table

		// IMPLEMENTATION NEEDED
		pt_entry_t *victim = coremap[frame].pte;

		if (victim->value & DIRTY){
			victim->swap_off = swap_pageout(frame, victim->swap_off);
			victim->value |= ONSWAP;
			evict_dirty_count += 1;
		}
		else {
			evict_clean_count += 1;
		}
		victim->value &= ~VALID;
		victim->value &= ~DIRTY;
	}

	// Record information for virtual page that will now be stored in frame
	coremap[frame].in_use = true;
	coremap[frame].pte = pte;

	return frame;
}

/*
 * Initializes your page table.
 * This function is called once at the start of the simulation.
 * For the simulation, there is a single "process" whose reference trace is
 * being simulated, so there is just one overall page table.
 *
 * In a real OS, each process would have its own page table, which would
 * need to be allocated and initialized as part of process creation.
 * 
 * The format of the page table, and thus what you need to do to get ready
 * to start translating virtual addresses, is up to you. 
 */
void init_pagetable(void)
{
	for (int i = 0; i < PT_SIZE; i++){
		pd[i].pt = NULL;
	}
}

pd_entry_t init_second_level(void)
{
	pd_entry_t* pd_s = malloc(PT_SIZE * sizeof(pd_entry_t));
	for (int i = 0; i < PT_SIZE; i++){
		pd_s[i].pt = NULL;
	}
	pd_entry_t new;
	new.pt = (vaddr_t) pd_s | VALID;

	return new;
}

pd_entry_t init_third_level(void)
{
	pt_entry_t* pt_t = malloc(PT_SIZE * sizeof(pd_entry_t));
	for (int i = 0; i < PT_SIZE; i++){
		pt_t[i].value = NULL;
		pt_t[i].swap_off = INVALID_SWAP;
	}
	pd_entry_t new;

	new.pt = (vaddr_t) pt_t | VALID;
	return new;
}

/*
 * Initializes the content of a (simulated) physical memory frame when it
 * is first allocated for some virtual address. Just like in a real OS, we
 * fill the frame with zeros to prevent leaking information across pages.
 */
static void init_frame(int frame)
{
	// Calculate pointer to start of frame in (simulated) physical memory
	unsigned char *mem_ptr = &physmem[frame * SIMPAGESIZE];
	memset(mem_ptr, 0, SIMPAGESIZE); // zero-fill the frame
}

/*
 * Locate the physical frame number for the given vaddr using the page table.
 *
 * If the page table entry is invalid and not on swap, then this is the first 
 * reference to the page and a (simulated) physical frame should be allocated 
 * and initialized to all zeros (using init_frame).
 *
 * If the page table entry is invalid and on swap, then a (simulated) physical 
 * frame should be allocated and filled by reading the page data from swap.
 *
 * When you have a valid page table entry, return the start of the page frame
 * that holds the requested virtual page.
 *
 * Counters for hit, miss and reference events should be incremented in
 * this function.
 */
unsigned char *find_physpage(vaddr_t vaddr, char type)
{
	int frame = -1; // Frame used to hold vaddr

	// To keep compiler happy - remove when you have a real use
	(void)vaddr;
	(void)type;
	(void)allocate_frame;
	(void)init_frame;

	// IMPLEMENTATION NEEDED
	vaddr_t top_index = PDPT_INDEX(vaddr);
	vaddr_t middle_index = PD_INDEX(vaddr);
	vaddr_t bottom_index = PT_INDEX(vaddr);

	// Use your page table to find the page table entry (pte) for the 
	// requested vaddr. 
	if (!(pd[top_index].pt & VALID)){
		pd[top_index] = init_second_level();
	}
	vaddr_t second_ptp = pd[top_index].pt;
	pd_entry_t * second_pt = (pd_entry_t*) (second_ptp & ~VALID); // reset the valid bit to get the second table

	if (!(second_pt[middle_index].pt & VALID)){
		second_pt[middle_index] = init_third_level();
	}
	vaddr_t third_ptp = second_pt[middle_index].pt;
	pt_entry_t* third_pt = (pt_entry_t*) (third_ptp & ~VALID); // reset the valid bit to get the last table 

	pt_entry_t pte = third_pt[bottom_index]; 

	// Check if pte is valid or not, on swap or not, and handle appropriately.
	// You can use the allocate_frame() and init_frame() functions here,
	// as needed.
	if (!(pte.value & VALID)){
		miss_count += 1;
		int frame = allocate_frame(&pte);
		if (pte.value & ONSWAP){
			// we read from swap file
			swap_pagein(frame, pte.swap_off);
			pte.value = frame << PT_SIZE;
			// set the flags of value
			pte.value |= ~ONSWAP;
			pte.value |= ~DIRTY;

		}
		else{
			// create a new one
			init_frame(frame, vaddr);
			pte.value = frame << PAGE_SHIFT;
			pte.value |= DIRTY; 
		}
	}
	else{
		hit_count += 1;
	}

	// Make sure that pte is marked valid and referenced. Also mark it
	// dirty if the access type indicates that the page will be written to.
	// (Note that a page should be marked DIRTY when it is first accessed, 
	// even if the type of first access is a read (Load or Instruction type).

	pte.value |= VALID | REF;

	if (type == "S" | type == "M") pte.value |= DIRTY;
	frame = &pte;

	// Call replacement algorithm's ref_func for this page.
	assert(frame != -1);
	ref_func(frame);

	// Return pointer into (simulated) physical memory at start of frame
	return &physmem[frame * SIMPAGESIZE];
}


void print_pagetable(void)
{
}


void free_pagetable(void)
{
}
