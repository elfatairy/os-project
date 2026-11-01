/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#include "../inc/mmu.h"

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [0] GIVEN FUNCTIONS
//==================================
// [1] INITIALIZE BUDDY FREE LISTS:
//==================================
void buddy_init()
{
	//panic("buddy_init is not implemented yet");

	for (int l = BUDDY_LOWER_LEVEL; l <= BUDDY_UPPER_LEVEL; ++l)
	{
		LIST_INIT(&(freeBlockLists[l]));
	}
}
//====================================
// [2] PRINT SIZE OF FREE LISTS:
//====================================
void PrintFreeBlockLists()
{
	for (int i = BUDDY_LOWER_LEVEL; i <= BUDDY_UPPER_LEVEL; ++i)
	{
		cprintf("Level#%d: %d\n", i, LIST_SIZE(&freeBlockLists[i]));
	}
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] GET BLOCK SIZE:
//===========================
__inline__ uint32 get_block_size(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [1] get_block_size

	 uint32 virt_addr =(uint32)va;
	 uint32 current_address = ROUNDDOWN(virt_addr,PAGE_SIZE);
	 uint32 page_end = current_address+PAGE_SIZE;

	 //Loop to find the needed block
	 while(current_address<page_end)
	  {
	   struct BlockMetaData *blk =(struct BlockMetaData*)current_address;
	   uint32 size=BLOCK_SIZE(blk->level);

	   if (virt_addr>= current_address && virt_addr <current_address + size)
	     return size;
	   // if we didn't find it, move to next block in the frame
	   current_address+= size;
	  }

	  return 0;

}
//===========================
// [2] ALLOCATE BLOCK:
//===========================
static uint32 find_needed_level(uint32 requiredSize) {
	uint32 total_required_size = requiredSize + META_DATA_SIZE;

	if (total_required_size > DYN_ALLOC_MAX_BLOCK_SIZE) {
		return -12;
	}
	int needed_level = 32 - __builtin_clz(total_required_size - 1);
	if (needed_level < BUDDY_LOWER_LEVEL) return BUDDY_LOWER_LEVEL;

	return needed_level;
}

static struct BlockMetaData *get_block(int level) {
	assert(level <= BUDDY_UPPER_LEVEL + 1 && level >= BUDDY_LOWER_LEVEL);

	if (level == BUDDY_UPPER_LEVEL + 1) {
		return (struct BlockMetaData *)create_buddy_space();
	}

	struct BlockList *list = &freeBlockLists[level];
	if (LIST_EMPTY(list)) {
		struct BlockMetaData *L_buddy = get_block(level + 1);
		if (L_buddy == NULL) return NULL;
		struct BlockMetaData *R_buddy = (struct BlockMetaData*)((uint32)L_buddy + BLOCK_SIZE(level)); //
		L_buddy->is_free = 1;
		L_buddy->level = level;
		R_buddy->is_free = 1;
		R_buddy->level = level;
		LIST_INSERT_TAIL(list, L_buddy);
		LIST_INSERT_TAIL(list, R_buddy);
	}
	struct BlockMetaData *selected_block = LIST_FIRST(list);
	LIST_REMOVE(list, selected_block);
	selected_block->is_free = 0;
	return selected_block;
}

void *alloc_block(uint32 requiredSize)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [2] alloc_block
	//Your code is here
	if (requiredSize == 0) return NULL;

	uint32 needed_level = find_needed_level(requiredSize);
	if (needed_level < 0) return NULL;
	struct BlockMetaData *block_start = get_block(needed_level);
	return (block_start == NULL ? NULL : (void *)((uint32)block_start + META_DATA_SIZE));
}

//===================================================
// [3] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [3] free_block
	//Your code is here
	//Comment the following line
	panic("free_block() not implemented");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//

//===========================
// [1] REALLOCATE BLOCK:
//===========================
void *realloc_block(void* va, uint32 newSize)
{
	//TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - realloc_block
	//Your code is here
	//Comment the following line
	panic("realloc_block() Not implemented yet");
}
