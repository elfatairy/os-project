#include "kheap.h"

#include "../conc/ksemaphore.h"
#include "../conc/kspinlock.h"
#include <inc/dynamic_allocator.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <kern/conc/sleeplock.h>
#include <kern/cpu/cpu.h>
#include <kern/mem/memory_manager.h>
#include <kern/proc/user_environment.h>

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS
//===================================//
//==================================================================================//

int move_data(uint32 old_va, uint32 new_va, uint32 o_size) {
  uint32 bytes_moved = 0;

  while (bytes_moved < o_size) {
    uint32 *old_ptr_page = NULL;
    int ret = get_page_table(ptr_page_directory, old_va, &old_ptr_page);
    if (ret == TABLE_IN_MEMORY && old_ptr_page != NULL) {
      int old_t_x = PTX(old_va);
      int PERM = (old_ptr_page[old_t_x] & 0xFFF);
      struct FrameInfo *current_frame =
          get_frame_info(ptr_page_directory, old_va, &old_ptr_page);
      if (current_frame != NULL) {
        map_frame(ptr_page_directory, current_frame, new_va, PERM);
        unmap_frame(ptr_page_directory, old_va);
      }
    }
    old_va += PAGE_SIZE;
    new_va += PAGE_SIZE;
    bytes_moved += PAGE_SIZE;
  }

  return 0;
}

int kheap_alloc_helper(uint32 va, uint32 size) {
  va = EXTRACT_ADDRESS(va);
  uint32 limit = va + size;

  while (va < limit) {
    struct FrameInfo *frame = NULL;
    if (allocate_frame(&frame) != 0) {
      return 1;
    }
    int perms = PERM_WRITEABLE;
    map_frame(ptr_page_directory, frame, va, perms);
    frame->va = (void *)va;

    va += PAGE_SIZE;
  }
  return 0;
}

struct AreaMetaData *create_area_metadata(void *start_address, uint32 size) {
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

void remove_area_metadata(struct AreaMetaData *area) { free_block(area); }

uint16 get_area_size(uint32 va) {
  uint32 page_number = (va - kheapPageAllocStart) / PAGE_SIZE;
  return area_sizes[page_number];
}
void set_area_size(uint32 va, uint16 page_count) {
  uint32 page_number = (va - kheapPageAllocStart) / PAGE_SIZE;
  area_sizes[page_number] = page_count;
}

// TODO: [PROJECT'26.GM#2] KERNEL HEAP - [0] GIVEN FUNCTIONS
//==============================================
//  [1] INITIALIZE KERNEL HEAP:
//==============================================
void kheap_init() {
  //==================================================================================
  // DON'T CHANGE THESE
  // LINES==========================================================
  //==================================================================================
  {
    buddy_init();
    init_kspinlock(&lk, "kheap lock");
    set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
    kheapPageAllocStart = KERNEL_HEAP_START;
    kheapPageAllocBreak = KERNEL_HEAP_START;
    LIST_INIT(&freeAreaList);
  }
  //==================================================================================
  //==================================================================================
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS
//==================================//
//==================================================================================//
/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/
int mark_buddy_space_page(uint32 va) {
  uint32 *ptr_page_table;

  if (get_page_table(ptr_page_directory, (uint32)va, &ptr_page_table) ==
      TABLE_NOT_EXIST) {
    return 1;
  }
  ptr_page_table[PTX(va)] = ptr_page_table[PTX(va)] | PERM_BSP;

  return 0;
}
int unmark_buddy_space_page(uint32 va) {
  uint32 *ptr_page_table;

  if (get_page_table(ptr_page_directory, (uint32)va, &ptr_page_table) ==
      TABLE_NOT_EXIST) {
    return 1;
  }
  ptr_page_table[PTX(va)] = ptr_page_table[PTX(va)] & (~PERM_BSP);

  return 0;
}

bool is_buddy_space_page(uint32 pte) { return ((pte & PERM_BSP) != 0); }

void *create_buddy_space() {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [4] create_buddy_space
  // Your code is here
  // Comment the following line
  //	panic("create_buddy_space() not implemented");
  release_kspinlock(&lk);
  void *buddy_va = kmalloc(PAGE_SIZE);
  acquire_kspinlock(&lk);
  if (buddy_va == NULL) {
    return NULL;
  }

  if (mark_buddy_space_page((uint32)buddy_va) != 0) {
    return NULL;
  }

  return buddy_va;
}

void delete_buddy_space(void *buddy_vaaa) {
  //   // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
  if (buddy_vaaa == NULL || (uint32)buddy_vaaa < KERNEL_HEAP_START || (uint32)buddy_vaaa >= KERNEL_HEAP_MAX) {
    return;
  }

  uint32 *ptr_page_table = NULL;

  if (get_page_table(ptr_page_directory, (uint32)buddy_vaaa, &ptr_page_table) == TABLE_NOT_EXIST) {
    return;
  }
  uint32 pte = ptr_page_table[PTX((uint32)buddy_vaaa)];
  if (!is_buddy_space_page(pte)) {
    return;
  }

  unmark_buddy_space_page((uint32)buddy_vaaa);
  unmap_frame(ptr_page_directory, (uint32)buddy_vaaa);
  //   // Comment the following line
  //   // panic("delete_buddy_space() not implemented");
}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
void *get_free_area(unsigned int size) {
  uint32 kheapPlacementStrategy = get_kheap_strategy();
  uint32 required_pages = size / PAGE_SIZE;

  if (kheapPlacementStrategy == KHP_PLACE_CUSTOMFIT) {
    struct AreaMetaData *el;
    struct AreaMetaData *used_area = NULL;

    LIST_FOREACH(el, &freeAreaList, AreaMetaData) {
      if (el->size == size) {
        used_area = el;
        break;
      } else if (el->size > size) {
        if ((used_area == NULL) || (el->size > used_area->size)) {
          used_area = el;
        }
      }
    }
    void *area_start = NULL;

    if (used_area == NULL) {
      if (KERNEL_HEAP_MAX - size < kheapPageAllocBreak) {
        return NULL;
      }

      area_start = (void *)kheapPageAllocBreak;
      kheapPageAllocBreak += size;
    } else {
      area_start = used_area->start_address;

      if (used_area->size == size) {
        LIST_REMOVE(&freeAreaList, used_area);
        remove_area_metadata(used_area);
      } else {
        used_area->start_address += size;
        used_area->size -= size;
      }
    }

    set_area_size((uint32)area_start, required_pages);
    return area_start;
  } else {
    panic("Kheap placement strategy: %c is not implemented",
          kheapPlacementStrategy);
  }
}

void get_next_prev_free_area(uint32 area_start, uint32 area_size, struct AreaMetaData **next_area, struct AreaMetaData **prev_area) {
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

void *kmalloc(unsigned int size) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
  // Your code is here
  // Comment the following line
  // kpanic_into_prompt("kmalloc() is not implemented yet...!!\n");
  acquire_kspinlock(&lk);

  if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
    void *va = alloc_block(size);
    release_kspinlock(&lk);
    return va;
  }

  uint32 required_size = ROUNDUP(size, PAGE_SIZE);

  void *area_start = get_free_area(required_size);
  if (area_start == NULL) {
    release_kspinlock(&lk);
    return NULL;
  }
  // kpanic_into_prompt("kmalloc() is not implemented yet...!!\n");

  // kheap_alloc_helper always return 0 but in case the panic in alloc_frame was
  // changed to non-zero return value in the future
  if (kheap_alloc_helper((uint32)area_start, required_size) != 0) {
    kfree(area_start);
    release_kspinlock(&lk);
    return NULL;
  }

  release_kspinlock(&lk);
  return area_start;
  /*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */
}

void kfree(void *virtual_address) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree

  acquire_kspinlock(&lk);
  if (virtual_address == NULL || (uint32)virtual_address < KERNEL_HEAP_START || (uint32)virtual_address >= KERNEL_HEAP_MAX) {
    release_kspinlock(&lk);
    panic("invalid address");
  }

  uint32 *ptr_page_table = NULL;
  if (get_page_table(ptr_page_directory, (uint32)virtual_address, &ptr_page_table) == TABLE_NOT_EXIST) {
    release_kspinlock(&lk);
    panic("this address not allocated");
  }

  uint32 pte = ptr_page_table[PTX((uint32)virtual_address)];
  if (is_buddy_space_page(pte)) {
    free_block(virtual_address);
    release_kspinlock(&lk);
    return;
  }

  uint32 current_area_NG = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
  uint32 num_of_pages = get_area_size(current_area_NG);
  if (num_of_pages == 0) {
    release_kspinlock(&lk);
    panic("this address not allocated");
  }

  uint32 size_of_area = num_of_pages * PAGE_SIZE;

  uint32 pages_freed = 0;
  for (uint32 start_point = current_area_NG; start_point < current_area_NG + size_of_area; start_point += PAGE_SIZE) {
    pages_freed++;
    unmap_frame(ptr_page_directory, start_point);
  }

  set_area_size(current_area_NG, 0);

  struct AreaMetaData *next_area = NULL;
  struct AreaMetaData *prev_area = NULL;
  get_next_prev_free_area(current_area_NG, size_of_area, &next_area, &prev_area);

  uint32 merged_area = current_area_NG;
  uint32 size_after_merge = size_of_area;

  if (next_area != NULL) {
    size_after_merge += next_area->size;
    LIST_REMOVE(&freeAreaList, next_area);
    remove_area_metadata(next_area);
  }

  if (prev_area != NULL) {
    merged_area = (uint32)prev_area->start_address;
    size_after_merge += prev_area->size;
    LIST_REMOVE(&freeAreaList, prev_area);
    remove_area_metadata(prev_area);
  }

  if (merged_area + size_after_merge == kheapPageAllocBreak) {
    kheapPageAllocBreak = merged_area;
    release_kspinlock(&lk);
    return;
  }

  struct AreaMetaData *my_new_merged_area_NG = create_area_metadata((void *)merged_area, size_after_merge);
  if (my_new_merged_area_NG == NULL) {
    release_kspinlock(&lk);
    panic("there is no enough memory for new area meta data");
  }
  LIST_INSERT_TAIL(&freeAreaList, my_new_merged_area_NG);
  release_kspinlock(&lk);

  //   // Comment the following line
  //   // panic("kfree() not implemented");
  //
  //   /*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #FREED PAGES) IS REQUIRED */
}
unsigned int kheap_physical_address(unsigned int virtual_address) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [8] kheap_physical_address
  // Your code is here
  // Comment the following line
  //	panic("kheap_physical_address() is not implemented yet...!!");
  if (virtual_address >= KERNEL_HEAP_MAX || virtual_address < KERNEL_HEAP_START) {
    return 0;
  }
  uint32 *ptr_page_table;
  if (get_page_table(ptr_page_directory, virtual_address, &ptr_page_table) == TABLE_NOT_EXIST) {
    return 0;
  }
  uint32 page_va = EXTRACT_ADDRESS(ptr_page_table[PTX(virtual_address)]);
  if (page_va == 0)
    return 0;

  return page_va + PGOFF(virtual_address);
  /*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

unsigned int kheap_virtual_address(unsigned int physical_address) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [9] kheap_virtual_address
  // Your code is here
  // Comment the following line
  //	panic("kheap_virtual_address() is not implemented yet...!!");

  struct FrameInfo *ptr_frame_info = to_frame_info(physical_address);

  if (ptr_frame_info->va == NULL)
    return 0;

  uint32 virtual_address = (uint32)ptr_frame_info->va;

  if (virtual_address >= KERNEL_HEAP_MAX ||
      virtual_address < KERNEL_HEAP_START) {
    return 0;
  }

  return ((uint32)ptr_frame_info->va) + PGOFF(physical_address);
  /*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION
//===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to
//"new_size" bytes, 	possibly moving it in the heap. 	If successful,
// returns the new virtual_address, in which case the old virtual_address must
// no
// longer be accessed. 	On failure, returns a null pointer, and the old
// virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().
void *krealloc(void *virtual_address, uint32 new_size) {
  // TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - krealloc
  // Your code is here
  // Comment the following line
  //	panic("krealloc() is not implemented yet...!!");

  //  	struct AreaMetaData *ele;
  //  	LIST_FOREACH(ele, freeAreaList,AreaMetaData){
  //  	}
  //  struct AreaMetaData *el;
  //  LIST_FOREACH(el, allocatedAreaList, AreaMetaData) {

  //  first check if virtual_address is within the kernal heap limits
  acquire_kspinlock(&lk);
  if ((uint32)virtual_address >= KERNEL_HEAP_MAX ||
      (uint32)virtual_address < KERNEL_HEAP_START) {
  }
  if (virtual_address == NULL) {
    release_kspinlock(&lk);
    void *start_add = kmalloc(new_size);
    return start_add;
  }

  uint32 old_size = get_area_size((uint32)virtual_address) * PAGE_SIZE;
  if (old_size == 0) {
    release_kspinlock(&lk);
    void *start_add = kmalloc(new_size);
    return start_add;
  }
  if (new_size == 0) {
    release_kspinlock(&lk);
    kfree(virtual_address);
    return NULL;
  }

  if (old_size == new_size) {
    release_kspinlock(&lk);
    return (void *)virtual_address;
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  if (new_size > old_size) {
    int t_p_index = PTX(virtual_address);
    uint32 *ptr_page_table;
    int ret = get_page_table(ptr_page_directory, (uint32)virtual_address,
                             &ptr_page_table);
    if (ret != TABLE_IN_MEMORY) {
      release_kspinlock(&lk);
      panic("something wrong in allocating page");
    }
    if (is_buddy_space_page(ptr_page_table[t_p_index])) {
      if (new_size >= DYN_ALLOC_MAX_BLOCK_SIZE) {

        release_kspinlock(&lk);
        uint32 new_va_dd = (uint32)kmalloc(new_size);
        acquire_kspinlock(&lk);

        move_data((uint32)virtual_address, new_va_dd, old_size);
        free_block(virtual_address);

        release_kspinlock(&lk);
        return (void *)new_va_dd;
        //

      } else {
        uint32 new_va_dd = (uint32)alloc_block(new_size);
        free_block(virtual_address);

        release_kspinlock(&lk);
        return (void *)new_va_dd;
      }
      ///////////////////////////////////////////////////////////////////////////////////////
    } else {
      struct AreaMetaData *el2 = NULL;
      uint32 dif_size = new_size - old_size;
      get_next_prev_free_area((uint32)virtual_address, dif_size, &el2, NULL);
      if (el2 == NULL) {
        release_kspinlock(&lk);
        uint32 new_v_add = (uint32)kmalloc(new_size);
        acquire_kspinlock(&lk);

        move_data((uint32)virtual_address, new_v_add, old_size);

        release_kspinlock(&lk);
        kfree(virtual_address);

        return (void *)new_v_add;
      } else {
        uint32 new_vadd = (uint32)el2->start_address;
        kheap_alloc_helper(new_vadd, new_size);
        set_area_size((uint32)virtual_address, new_size / PAGE_SIZE);

        release_kspinlock(&lk);
        return virtual_address;
      }
    }
  } else {
    int t_p_index = PTX(virtual_address);
    uint32 *ptr_page_table;
    int ret = get_page_table(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);

    if (ret != TABLE_IN_MEMORY) {
      release_kspinlock(&lk);
      panic("something wrong in allocating page");
    }
    if (is_buddy_space_page(ptr_page_table[t_p_index])) {
      void *new_vadd = alloc_block(new_size);
      free_block(virtual_address);

      release_kspinlock(&lk);
      return new_vadd;
    } else {
      if (new_size >= DYN_ALLOC_MAX_BLOCK_SIZE) {
        uint32 limit = (uint32)virtual_address + new_size;
        uint32 deleted_size = old_size - new_size;
        uint32 deleted_limit = limit + deleted_size;
        uint32 i = limit;
        while (i < deleted_limit) {
          unmap_frame(ptr_page_directory, limit);
          i += PAGE_SIZE;
        }
        struct AreaMetaData *newFreeArea = create_area_metadata((void *)limit, deleted_size);
        LIST_INSERT_TAIL(&freeAreaList, newFreeArea);
        set_area_size((uint32)virtual_address, (uint16)new_size / PAGE_SIZE);

        release_kspinlock(&lk);
        return virtual_address;
      } else {
        void *new_vadd = alloc_block(new_size);
        free_block(virtual_address);

        release_kspinlock(&lk);
        return new_vadd;
      }
    }
  }

  release_kspinlock(&lk);
  return NULL;
}
