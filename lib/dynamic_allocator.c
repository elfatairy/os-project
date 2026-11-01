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
static struct BlockMetaData *find_block_info(void *va) {
	uint32 virt_addr =(uint32)va;
	uint32 current_address = ROUNDDOWN(virt_addr,PAGE_SIZE);
	uint32 page_end = current_address+PAGE_SIZE;

	while(current_address<page_end)
	{
		struct BlockMetaData *blk =(struct BlockMetaData*)current_address;
		uint32 size=BLOCK_SIZE(blk->level);
		if (virt_addr>= current_address && virt_addr <current_address + size)
			return blk;
		current_address+= size;
	}
	return NULL;
}


__inline__ uint32 get_block_size(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [1] get_block_size
	struct BlockMetaData *that_block;
	if (that_block != NULL) return BLOCK_SIZE(that_block->level);
	panic("The memory is has error");
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

#define IS_LEFT(that_block) \
	({ \
		struct BlockMetaData *__that_block = (struct BlockMetaData *)that_block; \
		uint32 __shifted = __that_block >> __that_block->level; \
		!(__shifted & 1); \
	})


struct BlockMetaData *check_for_my_buddy(struct BlockMetaData *that_block){
	if (IS_LEFT(that_block)){
		return (struct BlockMetaData *)((uint32)that_block + BLOCK_SIZE(that_block->level));
	}
	else{
		return (struct BlockMetaData *)((uint32)that_block - BLOCK_SIZE(that_block->level));
	}
}

void Merge_Buddyzz(struct BlockMetaData *that_block){
	struct BlockMetaData *MyBuddy = check_for_my_buddy(that_block);
	if (MyBuddy->is_free == 0){
		return;
	}
	LIST_REMOVE(freeBlockLists[that_block->level],that_block);
	LIST_REMOVE(freeBlockLists[that_block->level],MyBuddy);

	struct BlockMetaData *Lovely_Merged_Buddyzz;
	if (IS_LEFT(that_block)){
		Lovely_Merged_Buddyzz = that_block;
	}
	else{
		Lovely_Merged_Buddyzz = MyBuddy;
	}

	Lovely_Merged_Buddyzz->is_free = 1;
	Lovely_Merged_Buddyzz->level = that_block->level +1;

	if (Lovely_Merged_Buddyzz->level == BUDDY_UPPER_LEVEL+1){
			delete_buddy_space(Lovely_Merged_Buddyzz);
			return;
		}

	LIST_INSERT_TAIL(freeBlockLists[Lovely_Merged_Buddyzz -> level],Lovely_Merged_Buddyzz);
	Merge_Buddyzz(Lovely_Merged_Buddyzz);
}

void free_block(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [3] free_block
	struct BlockMetaData *that_block;
	if (that_block->is_free == 0){
		that_block->is_free = 1;
		LIST_INSERT_TAIL(freeBlockLists[that_block->level],that_block);
		Merge_Buddyzz(that_block);
	}
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
