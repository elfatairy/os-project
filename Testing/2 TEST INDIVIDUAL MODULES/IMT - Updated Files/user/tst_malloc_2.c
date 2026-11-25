/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 10000 */
/* *********************************************************** */

#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

#define Mega  (1024*1024)
#define kilo (1024)
#define numOfAllocs 7
#define allocCntPerSize 200
#define sizeOfMetaData 8

extern volatile bool printStats;

void _main(void)
{
	printStats = 0;
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_active_list)) >= myEnv->page_WS_max_size)
			panic("InitialWSError1: Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif

	//1. Alloc some spaces
	int eval = 0;
	cprintf_colored(TEXT_cyan,"%~1. Alloc some spaces [50%]\n");
	{
		eval = initial_block_allocations();
		eval = ROUNDUP(eval * 50 / 100, 5); //rescale
	}
	//2. Fill-up the remaining pages using the 2KB block
	void *va;
	int is_correct;
	cprintf_colored(TEXT_cyan,"%~2. Fill-up the remaining pages using the 2KB block [15%]\n");
	{
		is_correct = 1;
		int freeFrames = (int)sys_calculate_free_frames() ;
		int usedDiskPagesBefore = (int)sys_pf_calculate_allocated_pages() ;
		int remainPagesInWS = myEnv->page_WS_max_size - LIST_SIZE(&(myEnv->page_WS_active_list)) ;
		int requiredNumOf2KBBlks = MIN(remainPagesInWS, ((USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE - numOfAllocPages)) * 2;
		int curIndex = BUDDY_UPPER_LEVEL;
		int curSize = DYN_ALLOC_MAX_BLOCK_SIZE - META_DATA_SIZE;
		//allocate the remaining 2KB block from last level
		{
			va = malloc(curSize);
		}
		numOfAllocBlocksPerSize[curIndex]++ ;
		//Fill-up the remaining pages in WS with 2KB alloc's
		{
			//malloc((USER_HEAP_MAX-USER_HEAP_START) - numOfAllocPages*PAGE_SIZE);
		}
		uint32 currentVA = USER_HEAP_START + numOfAllocPages*PAGE_SIZE ;
		int progressBarStep = requiredNumOf2KBBlks / 20 ;
		cprintf("%~\n\n>>>>>>>>>>>requiredNumOf2KBBlks = %d\n20#: ", requiredNumOf2KBBlks);
		for (int i = 1; i <= requiredNumOf2KBBlks; ++i)
		{
			if (i % progressBarStep == 0)
				cprintf("%~#");
			//allocate a new block with the curSize
			{
				va = malloc(curSize);
			}
			numOfAllocBlocksPerSize[curIndex]++ ;

			uint32 expectedVA = currentVA ;
			if (is_correct && ROUNDDOWN((uint32)va, PAGE_SIZE) != expectedVA)
			{
				is_correct = 0;
				cprintf_colored(TEXT_TESTERR_CLR, "Block Alloc #2.%d: WRONG! VA is not correct. Expected VA = %x, Actual VA = %x\n", i, expectedVA, ROUNDDOWN((uint32)va, PAGE_SIZE));
			}
			if (i%2 == 0)
			{
				numOfAllocPages++;
				currentVA += PAGE_SIZE;
			}
		}
		cprintf("%~\n");
		//Fill-up the remaining pages in UHEAP with one BIG alloc
		uint32 oneBigAllocSize = (USER_HEAP_MAX-USER_HEAP_START) - numOfAllocPages*PAGE_SIZE ;
		{
			malloc(oneBigAllocSize);
		}
		int usedDiskPagesAfter = (int)sys_pf_calculate_allocated_pages() ;
		if ((usedDiskPagesBefore - usedDiskPagesAfter) != 0) { is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }

		if (is_correct) eval += 10;
		is_correct = 1;
	}

	//3: Allocate the single remaining block from each level*/
	cprintf_colored(TEXT_cyan, "%~\n3: Allocate the single remaining block from each level\n\n") ;
	{
		is_correct = 1;
		int freeFrames = (int)sys_calculate_free_frames() ;
		int usedDiskPagesBefore = (int)sys_pf_calculate_allocated_pages() ;
		uint32 expectedVA = USER_HEAP_START ;
		for (int l = BUDDY_LOWER_LEVEL; l < BUDDY_UPPER_LEVEL; ++l) //block from last level already allocated in prev step
		{
			int curSize = BLOCK_SIZE(l) - META_DATA_SIZE ;
			//allocate a new block with the curSize
			{
				va = malloc(curSize);
			}

			numOfAllocBlocksPerSize[l]++ ;

			expectedVA += PAGE_SIZE;
			if (is_correct && ROUNDDOWN((uint32)va, PAGE_SIZE) != expectedVA)
			{
				is_correct = 0;
				cprintf_colored(TEXT_TESTERR_CLR, "%~Block Alloc #3: WRONG! VA is not correct. Expected VA = %x, Actual VA = %x\n",  expectedVA, ROUNDDOWN((uint32)va, PAGE_SIZE));
			}
		}
		int expectedNumOfAllocPages = 0;
		int usedDiskPagesAfter = (int)sys_pf_calculate_allocated_pages() ;
		if ((usedDiskPagesBefore - usedDiskPagesAfter) != 0) { is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != expectedNumOfAllocPages) { is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Wrong allocation: invalid number of allocated pages! Expected = %d, Actual = %d\n", expectedNumOfAllocPages, freeFrames - sys_calculate_free_frames()); }
		if (is_correct) eval += 10;
	}

	//4. Run out-of-heap memory
	cprintf_colored(TEXT_cyan,"%~4. Run out-of-heap memory [15%]\n");
	{
		is_correct = 1;
		int freeFrames = (int)sys_calculate_free_frames() ;
		int usedDiskPagesBefore = (int)sys_pf_calculate_allocated_pages() ;
		for (int l = BUDDY_LOWER_LEVEL; l <= BUDDY_UPPER_LEVEL; ++l)
		{
			int curSize = BLOCK_SIZE(l) - META_DATA_SIZE ;
			//allocate a new block with the curSize
			{
				va = malloc(curSize);
			}
			//cprintf("va = kmalloc(%d) => %x\n", curSize, va);
			uint32 expectedVA = 0;
			if (is_correct && ROUNDDOWN((uint32)va, PAGE_SIZE) != expectedVA)
			{
				is_correct = 0;
				cprintf_colored(TEXT_TESTERR_CLR, "%~Block Alloc #4: WRONG! VA is not correct. Expected VA = %x, Actual VA = %x\n",  expectedVA, ROUNDDOWN((uint32)va, PAGE_SIZE));
			}
		}

		int expectedNumOfAllocPages = 0;
		int usedDiskPagesAfter = (int)sys_pf_calculate_allocated_pages() ;
		if ((usedDiskPagesBefore - usedDiskPagesAfter) != 0) { is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != expectedNumOfAllocPages) { is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"Wrong allocation: invalid number of allocated pages! Expected = %d, Actual = %d\n", expectedNumOfAllocPages, freeFrames - sys_calculate_free_frames()); }
		if (is_correct) eval += 20;
	}

	//5. Check Permissions
	cprintf_colored(TEXT_cyan,"%~\n5. Check permissions of allocated spaces in BLOCK ALLOCATOR [10%]\n");
	{
		is_correct = 1;
		uint32 lastAllocAddress = (uint32)ptr_allocations[8] + 2*Mega ;
		uint64 va;
		uint32 endVA = 	USER_HEAP_START + numOfAllocPages*PAGE_SIZE ;

		for (va = USER_HEAP_START; va < endVA; va+=PAGE_SIZE)
		{
			unsigned int * table;
			uint32 perm = vpt[VPN(va)] & 0xFFF;
			if ((perm & PERM_USER) != PERM_USER || (perm & PERM_WRITEABLE) != PERM_WRITEABLE || (perm & PERM_UHPAGE) != PERM_UHPAGE)
			{
				if (is_correct)
				{
					is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Block Alloc #5: Wrong permissions: pages should be mapped with user permission, writeable & marked as user heap\n");
				}
			}
		}
		if (is_correct) eval += 10;
	}

	cprintf_colored(TEXT_light_green,"%~\nTest malloc Block Alloc Completed. Evaluation = %d%\n", eval);
}
