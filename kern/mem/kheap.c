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
            struct FrameInfo *current_frame = get_frame_info(ptr_page_directory, old_va, &old_ptr_page);
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
  struct AreaMetaData *area =
      (struct AreaMetaData *)alloc_block(sizeof(struct AreaMetaData));

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
  void *buddy_va = kmalloc(PAGE_SIZE);
  if (buddy_va == NULL) {
    return NULL;
  }

  if (mark_buddy_space_page((uint32)buddy_va) != 0) {
    return NULL;
  }

  return buddy_va;
}

void delete_buddy_space(void *va) {
//  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
//
//  uint32 *ptr_page_table = NULL;
//  get_page_table(ptr_page_directory, va,
//                 &ptr_page_table) if (ptr_page_table == NULL) return;
//  uint32 entry = ptr_page_table[PTX(va)];
//  if (!is_buddy_space_page(entry))
//    panic("this page is not a buddy space");
//  else
//    unmap_frame(ptr_page_directory, va);
//
//  // Comment the following line
//  // panic("delete_buddy_space() not implemented");
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

void get_next_prev_free_area(uint32 area_start, uint32 area_size,
                             struct AreaMetaData **next_area,
                             struct AreaMetaData **prev_area) {

  struct AreaMetaData *el;
  uint32 next_area_start_address = area_start + area_size;

  LIST_FOREACH(el, &freeAreaList, AreaMetaData) {
    if (((uint32)el->start_address) == next_area_start_address) {
      *next_area = el;
    } else if (((uint32)el->start_address + el->size) == area_start) {
      *prev_area = el;
    }
    if (next_area != NULL && prev_area != NULL)
      return;
  }
}

void *kmalloc(unsigned int size) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
  // Your code is here
  // Comment the following line
  // kpanic_into_prompt("kmalloc() is not implemented yet...!!\n");
  if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
    return alloc_block(size);
  }

  uint32 required_size = ROUNDUP(size, PAGE_SIZE);

  void *area_start = get_free_area(required_size);
  if (area_start == NULL) {
    return NULL;
  }
  // kpanic_into_prompt("kmalloc() is not implemented yet...!!\n");

  // kheap_alloc_helper always return 0 but in case the panic in alloc_frame was
  // changed to non-zero return value in the future
  if (kheap_alloc_helper((uint32)area_start, required_size) != 0) {
    kfree(area_start);
    return NULL;
  }

  return area_start;
  /*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */
}

void kfree(void *virtual_address) {
//  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree
//
//  // first check if virtual_address is within the kernal heap limits
//
//  struct AreaMetaData *current_area = NULL;
//  struct AreaMetaData *temp = NULL;
//  LIST_FOREACH(current_area, &allocatedAreaList, AreaMetaData) {
//    if (current_area->start_address == virtual_address)
//      current_area = temp;
//    break;
//  }
//
//  if (current_area == NULL)
//    return;
//
//  LIST_REMOVE(&allocatedAreaList, current_area);
//
//  struct AreaMetaData *next_free_area = NULL;
//  struct AreaMetaData *prev_free_area = NULL;
//
//  get_next_prev_free_area((uint32)current_area->start_address,
//                          current_area->size, &next_free_area,
//                          &prev_free_area);
//
//  if (current_area->size > PAGE_SIZE) {
//
//    if (next_free_area != NULL) {
//      current_area->size += next_free_area->size;
//    }
//
//    if (prev_free_area != NULL) {
//      current_area->size += prev_free_area->size;
//      current_area->start_address = prev_free_area->start_address;
//    }
//
//    uint32 free_area = current_area->start_address;
//    uint32 free_frame = free_area + current_area->size;
//
//    for (; free_area < free_frame; free_area += PAGE_SIZE)
//      unmap_frame(*ptr_page_directory, free_area);
//  }
//
//  // merged done
//
//  // Comment the following line
//  // panic("kfree() not implemented");
//
//  /*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #FREED PAGES) IS REQUIRED */
}

unsigned int kheap_physical_address(unsigned int virtual_address) {
  // TODO: [PROJECT'26.GM#2] KERNEL HEAP - [8] kheap_physical_address
  // Your code is here
  // Comment the following line
  //	panic("kheap_physical_address() is not implemented yet...!!");
  if (virtual_address >= KERNEL_HEAP_MAX ||
      virtual_address < KERNEL_HEAP_START) {
    return 0;
  }
  uint32 *ptr_page_table;
  if (get_page_table(ptr_page_directory, virtual_address, &ptr_page_table) ==
      TABLE_NOT_EXIST) {
    return 0;
  }
  return EXTRACT_ADDRESS(ptr_page_table[PTX(virtual_address)]) +
         PGOFF(virtual_address);
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
//	if (virtual_address >= KERNEL_HEAP_MAX ||virtual_address < KERNEL_HEAP_START){
//
//	}
  if (virtual_address == NULL) {
    void *start_add = kmalloc(new_size);
    return start_add;
  }


    uint32 old_size =(uint32) get_area_size( (uint32) virtual_address) * PAGE_SIZE;
    if(old_size == 0 ){
        void *start_add = kmalloc(new_size);
        return start_add;
    }
      if (new_size == 0) {
        kfree(virtual_address);
        return NULL;
      }

      if (old_size == new_size) {
        return (void *)virtual_address;
      }
//////////////////////////////////////////////////////////////////////////////////////////
      if (new_size > old_size) {
        int t_p_index = PTX(virtual_address);
        uint32 *ptr_page_table;
        int ret = get_page_table(ptr_page_directory, (uint32) virtual_address,&ptr_page_table);
        if (ret != TABLE_IN_MEMORY) {
          panic("something wrong in allocating page");
        }
        if (is_buddy_space_page(ptr_page_table[t_p_index])) {
          if (new_size >= DYN_ALLOC_MAX_BLOCK_SIZE) {

            uint32 new_va_dd = (uint32)kmalloc(new_size);
            move_data((uint32) virtual_address,new_va_dd,old_size);
            free_block(virtual_address);
            return (void *)new_va_dd;
            //

          } else {
            uint32 new_va_dd = (uint32)alloc_block(new_size);
            free_block(virtual_address);
            return (void *)new_va_dd;
          }
          ///////////////////////////////////////////////////////////////////////////////////////
        } else {
          struct AreaMetaData *el2 = NULL;
          uint32 dif_size = new_size - old_size;
          get_next_prev_free_area((uint32) virtual_address, dif_size, &el2 ,NULL);
          if (el2 == NULL) {
            uint32 new_v_add = (uint32) kmalloc(new_size);
            move_data((uint32) virtual_address, new_v_add, old_size);
            kfree(virtual_address);
            return (void *)new_v_add;
          } else {
            uint32 new_vadd = (uint32) el2->start_address;
            kheap_alloc_helper(new_vadd, new_size);
            set_area_size((uint32) virtual_address,new_size/PAGE_SIZE);
            return virtual_address;
          }
        }
      } else {
          int t_p_index = PTX(virtual_address);
          uint32 *ptr_page_table;
          int ret = get_page_table(ptr_page_directory, (uint32) virtual_address,&ptr_page_table);
          if (ret != TABLE_IN_MEMORY) {
            panic("something wrong in allocating page");
          }
        if (is_buddy_space_page(ptr_page_table[t_p_index])) {
          void * new_vadd = alloc_block(new_size);
          free_block(virtual_address);
          return new_vadd;
        } else {
          if (new_size >= DYN_ALLOC_MAX_BLOCK_SIZE) {
            uint32 limit = (uint32) virtual_address + new_size;
            uint32 deleted_size = old_size - new_size;
            uint32 deleted_limit = limit + deleted_size;
            uint32 i = limit ;
            while (i < deleted_limit) {
              unmap_frame(ptr_page_directory, limit);
              i += PAGE_SIZE;
            }
            struct AreaMetaData *newFreeArea = create_area_metadata((void *)limit, deleted_size);
            LIST_INSERT_TAIL(&freeAreaList, newFreeArea);
            set_area_size((uint32)virtual_address, (uint16) new_size/PAGE_SIZE);
            return virtual_address;
          } else {
            void * new_vadd = alloc_block(new_size);
            free_block(virtual_address);
            return new_vadd;
          }
        }
      }

//  }
  return NULL;
}
