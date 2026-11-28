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

int mark_buddy_space_page(uint32 va) {
	  if (va < USER_HEAP_START || va >= USER_HEAP_MAX)
	    return 1; //The VA is is not in the boundries of the heap

	  uint32 i = uheap_page_index(va);
	  buddy_page_bitmap[i / 32] |= (1U << (i % 32));
	  return 0;
}
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


//=================================
// [1] CREATE NEW BUDDY SPACE:
//=================================

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

struct AreaMetaData {
	void *start_address;
	uint32 size;
	LIST_ENTRY(AreaMetaData) prev_next_info;
} __attribute__((packed));
LIST_HEAD(AreaList, AreaMetaData);
struct AreaList freeAreaList;
uint16 area_sizes[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE];
struct AreaMetaData *create_area_metadata(void *start_address, uint32 size)
{
	struct AreaMetaData *area = (struct AreaMetaData *)alloc_block(sizeof(struct AreaMetaData));
	if (area == NULL) {
		return NULL;
	}

	area->start_address = start_address;
	area->size = size;
	area->prev_next_info.le_next = NULL;
	area->prev_next_info.le_prev = NULL;
	return area;
}
void remove_area_metadata(struct AreaMetaData *area)
{
	free_block(area);
}
uint16 get_area_size(uint32 va)
{
	uint32 page_number = (va - uheapPageAllocStart) / PAGE_SIZE;
	return area_sizes[page_number];
}
void set_area_size(uint32 va, uint16 page_count)
{
	uint32 page_number = (va - uheapPageAllocStart) / PAGE_SIZE;
	area_sizes[page_number] = page_count;
}
void get_next_prev_free_area(uint32 area_start, uint32 area_size, struct AreaMetaData **next_area, struct AreaMetaData **prev_area)
{
	*prev_area = NULL;
	*next_area = NULL;
	struct AreaMetaData *el;
	uint32 next_area_start_address = area_start + area_size;

	LIST_FOREACH(el, &freeAreaList, AreaMetaData) {
		if (((uint32)el->start_address) == next_area_start_address) {
			*next_area = el;
		}
		if (((uint32)el->start_address + el->size) == area_start) {
			*prev_area = el;
		}
		if (*next_area != NULL && *prev_area != NULL)
			return;
	}
}
void *get_free_area(uint32 size)
{
	uint32 required_pages = size / PAGE_SIZE;

	if (uheapPlaceStrategy == UHP_PLACE_CUSTOMFIT) {
		struct AreaMetaData *element;
		struct AreaMetaData *used_area = NULL;

		LIST_FOREACH(element, &freeAreaList, AreaMetaData) {
			if (element->size == size) {
				used_area = element;
				break;
			}
			else if (element->size > size) {
				if ((used_area == NULL) || (element->size > used_area->size)) {
					used_area = element;
				}
			}
		}

		void *area_start = NULL;

		if (used_area == NULL) {
			if (USER_HEAP_MAX - size < uheapPageAllocBreak) {
				return NULL;
			}
			area_start = (void *)uheapPageAllocBreak;
			uheapPageAllocBreak = uheapPageAllocBreak + size;
		}

		else {
			area_start = used_area->start_address;
			if (used_area->size == size) {
				LIST_REMOVE(&freeAreaList, used_area);
				remove_area_metadata(used_area);
			}

			else {
				used_area->start_address = used_area->start_address + size;
				used_area->size = used_area->size - size;
			}
		}
		set_area_size((uint32)area_start, required_pages);
		return area_start;
	}
	else {
		panic("Uheap placement strategy: %c is not implemented", uheapPlaceStrategy);
	}
}


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
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
			return alloc_block(size);
		}

	uint32 required_size = ROUNDUP(size, PAGE_SIZE);

	void *area_start = get_free_area(required_size);
		if (area_start == NULL) {
			return NULL;
		}

	sys_allocate_user_mem((uint32)area_start, required_size);

	return area_start;
	//Comment the following line
	//panic("malloc() is not implemented yet...!!");
}

//=================================
// [4] FREE SPACE FROM USER HEAP:
//=================================

void free(void* virtual_address)
{
	//TODO: [PROJECT'26.IM#2] USER HEAP - [5] free
	//Your code is here
	if (is_buddy_space_page((uint32)virtual_address)) {
		free_block(virtual_address);
		return;
	}

	if ((uint32)virtual_address < uheapPageAllocStart || (uint32)virtual_address >= uheapPageAllocBreak) {
		panic("invalid address");
	}

	uint32 area_start = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);

	uint32 num_of_pages = get_area_size(area_start);

	if (num_of_pages == 0) {
		panic("this address not allocated");
	}
	uint32 size_of_area = num_of_pages * PAGE_SIZE;

	struct AreaMetaData* spare = create_area_metadata(NULL, 0);
	if (spare == NULL) {
		panic("there is no enough memory for new area meta data");
	}
	set_area_size(area_start, 0);

	sys_free_user_mem(area_start, size_of_area);

	struct AreaMetaData *next_area = NULL;
	struct AreaMetaData *prev_area = NULL;

	get_next_prev_free_area(area_start, size_of_area, &next_area, &prev_area);

	uint32 merged_area = area_start;

	uint32 size_after_merge = size_of_area;

	if (next_area != NULL) {
		size_after_merge += next_area->size;
		LIST_REMOVE(&freeAreaList, next_area);
	}

	if (prev_area != NULL) {
		merged_area = (uint32)prev_area->start_address;
		size_after_merge += prev_area->size;
		LIST_REMOVE(&freeAreaList, prev_area);
	}

	/* BLANK 3: lower the break, or fill the spare entry and add it to the list */
	if (merged_area + size_after_merge == uheapPageAllocBreak) {
		uheapPageAllocBreak = merged_area;
	}
	else {
		spare->start_address = (void *)merged_area;
		spare->size = size_after_merge;
		LIST_INSERT_TAIL(&freeAreaList, spare);
		spare = NULL;
	}
	if (next_area != NULL) {
		remove_area_metadata(next_area);
	}

	if (prev_area != NULL) {
		remove_area_metadata(prev_area);
	}

	if (spare != NULL) {
		remove_area_metadata(spare);
	}

	//Comment the following line
	//panic("free() is not implemented yet...!!");
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
