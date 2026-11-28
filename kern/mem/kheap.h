#ifndef FOS_KERN_KHEAP_H_
#define FOS_KERN_KHEAP_H_

#ifndef FOS_KERNEL
#error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/queue.h>
#include <inc/types.h>

/*2017*/
// Values for user heap placement strategy
#define KHP_PLACE_CONTALLOC 0x0
#define KHP_PLACE_FIRSTFIT 0x1
#define KHP_PLACE_BESTFIT 0x2
#define KHP_PLACE_NEXTFIT 0x3
#define KHP_PLACE_WORSTFIT 0x4
#define KHP_PLACE_CUSTOMFIT 0x5

/******************************************************************************/
// TODO: [PROJECT'26.GM#2] KERNEL HEAP - [0] GIVEN DATA
uint32 kheapPageAllocStart;
uint32 kheapPageAllocBreak;
uint32 kheapPlacementStrategy;
struct AreaMetaData {
  void *start_address;
  uint32 size;
  LIST_ENTRY(AreaMetaData) prev_next_info; /* linked list links */
} __attribute__((packed));

struct kspinlock lk;
uint16 area_sizes[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE];

LIST_HEAD(AreaList, AreaMetaData); // Declares 'struct BlockList'
struct AreaList freeAreaList;
/******************************************************************************/
// Helpers
int move_data(uint32 old_va, uint32 new_va, uint32 o_size);
int kheap_alloc_helper(uint32 va, uint32 size);

int mark_buddy_space_page(uint32 va);
int unmark_buddy_space_page(uint32 va);
bool is_buddy_space_page(uint32 pte);

struct AreaMetaData *create_area_metadata(void *start_address, uint32 size);
void remove_area_metadata(struct AreaMetaData *area);

uint16 get_area_size(uint32 va);s
void set_area_size(uint32 va, uint16 page_count);

void *get_free_area(unsigned int size);
void get_next_prev_free_area(uint32 area_start, uint32 area_size,
                             struct AreaMetaData **next_area,
                             struct AreaMetaData **prev_area);

/*2025*/ // Replaced by setter & getter function
static inline void set_kheap_strategy(uint32 strategy) {
  kheapPlacementStrategy = strategy;
}
static inline uint32 get_kheap_strategy() { return kheapPlacementStrategy; }

//***********************************
void kheap_init();

void *kmalloc(unsigned int size);
void kfree(void *virtual_address);
void *krealloc(void *virtual_address, unsigned int new_size);

unsigned int kheap_virtual_address(unsigned int physical_address);
unsigned int kheap_physical_address(unsigned int virtual_address);

int numOfKheapVACalls;

#endif // FOS_KERN_KHEAP_H_
