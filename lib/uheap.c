#include <inc/lib.h>

#define UHEAP_NUM_PAGES ((USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE)
static uint32 buddy_page_bitmap[UHEAP_NUM_PAGES / 32];
static inline uint32 uheap_page_index(uint32 va) {
  return (va - USER_HEAP_START) / PAGE_SIZE;
}


/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.IM#2] USER HEAP - [0] GIVEN FUNCTION
int __firstTimeFlag = 1;
void uheap_init()
{
	if(__firstTimeFlag)
	{
		buddy_init();
		uheapPlaceStrategy = sys_get_uheap_strategy();
		uheapPageAllocStart = USER_HEAP_START;
		uheapPageAllocBreak = USER_HEAP_START;
		__firstTimeFlag = 0;
	}
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/



//=================================
// [1] CREATE NEW BUDDY SPACE:
//=================================
int mark_buddy_space_page(uint32 va) {
	  if (va < USER_HEAP_START || va >= USER_HEAP_MAX)
	    return 1; //The VA is is not in the boundries of the heap

	  uint32 i = uheap_page_index(va);
	  buddy_page_bitmap[i / 32] |= (1U << (i % 32));
	  return 0;
}

void* create_buddy_space()
{
	//TODO: [PROJECT'26.IM#2] USER HEAP - [1] create_buddy_space
	void *buddy_va = malloc(PAGE_SIZE);
	if (buddy_va == NULL) {
		return NULL;
	}
	int Marked = mark_buddy_space_page((uint32)buddy_va);

	if (Marked!=0) {
		free(buddy_va);
		return NULL;
	}
	return buddy_va;
	//Comment the following line
	//panic("create_buddy_space() is not implemented yet...!!");
}

//=================================
// [2] DELETE EXISTING BUDDY SPACE:
//=================================

int unmark_buddy_space_page(uint32 va) {
	if (va < USER_HEAP_START || va >= USER_HEAP_MAX)
	    return 1; //The VA is is not in the boundries of the heap

	  uint32 i = uheap_page_index(va);
	  buddy_page_bitmap[i / 32] &= ~(1U << (i % 32));
	  return 0;
}

bool is_buddy_space_page(uint32 va) {
	 if (va < USER_HEAP_START || va >= USER_HEAP_MAX)
	    return 0;
	  uint32 i = uheap_page_index(va);
	  return (buddy_page_bitmap[i / 32] >> (i % 32)) & 1;
}

void  delete_buddy_space(void *va)
{
	//TODO: [PROJECT'26.IM#2] USER HEAP - [2] delete_buddy_space
	//Your code is here
	if (va == NULL || (uint32)va < USER_HEAP_START || (uint32)va >= USER_HEAP_MAX) {
		return;
	}
	if (!is_buddy_space_page((uint32)va)) {
		return;
	}

	unmark_buddy_space_page((uint32)va);

	free(va);
	//Comment the following line
	//panic("delete_buddy_space() is not implemented yet...!!");


}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
//=================================
// [3] ALLOCATE SPACE IN USER HEAP:
//=================================

void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	uheap_init();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'26.IM#2] USER HEAP - [3] malloc
	//Your code is here
	//Comment the following line
	panic("malloc() is not implemented yet...!!");
}

//=================================
// [4] FREE SPACE FROM USER HEAP:
//=================================



void free(void* virtual_address)
{
	//TODO: [PROJECT'26.IM#2] USER HEAP - [5] free
	//Your code is here
	//Comment the following line
	panic("free() is not implemented yet...!!");
}

//=================================
// [5] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	uheap_init();
	if (size == 0) return NULL ;
	//==============================================================

	//TODO: [PROJECT'26.IM#6] SHARED MEMORY - [3] smalloc
	//Your code is here
	//Comment the following line
	panic("smalloc() is not implemented yet...!!");
}

//========================================
// [6] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	uheap_init();
	//==============================================================

	//TODO: [PROJECT'26.IM#6] SHARED MEMORY - [5] sget
	//Your code is here
	//Comment the following line
	panic("sget() is not implemented yet...!!");
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//
//=================================
// FREE SHARED VARIABLE:
//=================================
void sfree(void* virtual_address)
{
	//TODO: [PROJECT'26.BONUS#4] EXIT #2 - [3] Free Shared Variables (sfree)
	//Your code is here
	//Comment the following line
	panic("sfree() is not implemented yet...!!");
}
