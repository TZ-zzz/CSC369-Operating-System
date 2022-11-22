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

static pd_entry_t pdpt[PT_SIZE];

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
	for (size_t i = 0; i < memsize; i++) {
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
		pt_entry_t * victim = coremap[frame].pte;
		if (victim->value & DIRTY){
			int new_swap_offset = swap_pageout(frame, victim->swap_off);
			if (new_swap_offset == INVALID_SWAP)
			{
				exit(1);
			}
			victim->swap_off = new_swap_offset;
			evict_dirty_count += 1;
			victim->value &= ~DIRTY;
			victim->value |= ONSWAP;
		}
		else{

			evict_clean_count += 1;
		}
		victim->value &= ~VALID;
		// victim->value |= ONSWAP;
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
		pdpt[i].pt = 0;
	}
}

static pd_entry_t init_second_level(void)
{
	pd_entry_t* pd = malloc(PT_SIZE * sizeof(pd_entry_t));
	for (int i = 0; i < PT_SIZE; i++){
		pd[i].pt = 0;
	}
	pd_entry_t  new;
	new.pt = (vaddr_t) pd | VALID;

	return new;
}

static pd_entry_t init_third_level(void)
{
	pt_entry_t* pt = malloc(PT_SIZE * sizeof(pt_entry_t));
	for (int i = 0; i < PT_SIZE; i++){
		pt[i].value = 0;
		pt[i].swap_off = INVALID_SWAP;
	}
	pd_entry_t new;

	new.pt = (vaddr_t) pt | VALID;
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
	// (void)vaddr;
	// (void)type;
	// (void)allocate_frame;
	// (void)init_frame;

	// IMPLEMENTATION NEEDED
	vaddr_t top_index = (vaddr >> 36);
	vaddr_t middle_index = (vaddr >> 24) & PT_MASK;
	vaddr_t bottom_index = (vaddr >> 12) & PT_MASK;

	// Use your page table to find the page table entry (pte) for the 
	// requested vaddr. 
	if (!(pdpt[top_index].pt & VALID)){
		pdpt[top_index] = init_second_level();
	}
	vaddr_t second_ptp = pdpt[top_index].pt;
	pd_entry_t *second_pt = (pd_entry_t *)(second_ptp & ~VALID); //mask the valid bit to get the addr

	// Use 2nd-level page table and vaddr to get pointer to 3rd-level page table
	if (!(second_pt[middle_index].pt & VALID)){
		second_pt[middle_index] = init_third_level();
	}
	vaddr_t third_ptp = second_pt[middle_index].pt;
	pt_entry_t *third_pt = (pt_entry_t *)(third_ptp & ~VALID);

	// Use 3rd-level page table and vaddr to get pointer to page table entry
	pt_entry_t* pte = &(third_pt[bottom_index]);

	// Check if pte is valid or not, on swap or not, and handle appropriately
	// (Note that the first acess to a page should be marked DIRTY)
	// Make sure that pte is marked valid and referenced. Also mark it
	// dirty if the access type indicates that the page will be written to.
	if (!(pte->value & VALID))
	{
		miss_count += 1;
		frame = allocate_frame(pte);
		if (pte->value & ONSWAP){
			swap_pagein(frame, pte->swap_off);
			pte->value = frame << PAGE_SHIFT;
			pte->value |= ONSWAP;
		}
		else{
			init_frame(frame);
			pte->value = frame << PAGE_SHIFT;
			pte->value |= DIRTY;
		}
		pte->value |= VALID;
	}
	else
	{
		hit_count += 1;
		frame = pte->value >> PAGE_SHIFT;
	}

	// Make sure that pte is marked valid and referenced. Also mark it
	// dirty if the access type indicates that the page will be written to.
	// (Note that a page should be marked DIRTY when it is first accessed, 
	// even if the type of first access is a read (Load or Instruction type).

	pte->value |= REF;

	if ((type == 'S') | (type == 'M')) {
		pte->value |= DIRTY;
	}

	// Call replacement algorithm's ref_func for this page.
	ref_count += 1;
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

