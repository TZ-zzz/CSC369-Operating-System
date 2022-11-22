#ifndef __PAGETABLE_H__
#define __PAGETABLE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>


// User-level virtual addresses on a 64-bit Linux system are 48 bits in our
// traces, and the page size is 4096 (12 bits). The remaining 36 bits are
// the virtual page number, which is used as the lookup key (or index) into
// your page table. 


// Page table entry 
// This structure will need to record the physical page frame number
// for a virtual page, as well as the swap offset if it is evicted. 
// You will also need to keep track of the Valid, Dirty and Referenced
// status bits (or flags). 
// You do not need to keep track of Read/Write/Execute permissions.
#define VALID 0x1 // first bit
#define DIRTY 0x2 // second bit
#define REF 0x4 //third bit
#define ONSWAP 0x8 //fourth bit
#define PFN_OFFSET 12

#define PT_SIZE (2^12)

typedef struct pt_entry_s {
	// Add any fields you need ...
	int value;
	off_t swap_off;
} pt_entry_t;

typedef struct pd_entry {
	vaddr_t pt;
} pd_entry_t;

#endif /* __PAGETABLE_H__ */
