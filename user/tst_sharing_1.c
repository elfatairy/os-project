// Test the creation of shared variables (create_shared_memory)
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>
extern volatile bool printStats;

void
_main(void)
{
	printStats = 0;

	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL BLOCK ALLOCATOR OR USER
	 * BLOCK ALLOCATOR DUE TO DIFFERENT MANAGEMENT OF USER HEAP
	 *********************************************************/

	cprintf_colored(TEXT_yellow, "%~************************************************\n");
	cprintf_colored(TEXT_yellow, "%~MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf_colored(TEXT_yellow, "%~************************************************\n\n\n");

	/*=================================================*/
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_active_list)) >= myEnv->page_WS_max_size)
			panic("InitialWSError1: Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif
	/*=================================================*/

	int eval = 0;
	bool is_correct = 1;
	int extraPages = 1;
	uint32 *x, *y, *z ;
	uint32 expected ;
	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS
	int freeFrames, usedDiskPages ;

	cprintf_colored(TEXT_cyan, "\n%~STEP A: checking the creation of shared variables... [45%]\n");
	{
		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		x = smalloc("x", PAGE_SIZE, 1);
		if (x != (uint32*)pagealloc_start)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 1+1 ; /*1page +1table*/
		int diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected + extraPages /*KH Block: 1 page for Buddy + UH Block: Max 1 page for Buddy (if any)*/ ))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Wrong allocation (actual=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expected +1 +2);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Wrong page file allocation: ");}
		if (diff == expected + extraPages) extraPages = 0; //if extraPage is allocated for KH & UH, then no extra will be expected since there'll be free blocks in Block Allo since last allocation
		else if (diff == expected + extraPages - 1) extraPages = 1; //if extraPages is allocated for either KH or UH, then max of one extra will be expected
		if (is_correct) eval += 15 ;

		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		z = smalloc("z", PAGE_SIZE + 4, 1);
		if (z != (uint32*)(pagealloc_start + 1 * PAGE_SIZE))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 2 ; /*2 pages*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected+extraPages))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Wrong page file allocation: ");}
		if (diff == expected + extraPages) extraPages = 0; //if extraPage is allocated for KH & UH, then no extra will be expected since there'll be free blocks in Block Allo since last allocation
		else if (diff == expected + extraPages - 1) extraPages = 1; //if extraPages is allocated for either KH or UH, then max of one extra will be expected
		if (is_correct) eval += 15 ;

		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		y = smalloc("y", 4, 1);
		if (y != (uint32*)(pagealloc_start + 3 * PAGE_SIZE))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 1 ; /*1 page*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected+extraPages))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Wrong page file allocation: ");}
		if (diff == expected + extraPages) extraPages = 0; //if extraPage is allocated for KH & UH, then no extra will be expected since there'll be free blocks in Block Allo since last allocation
		else if (diff == expected + extraPages - 1) extraPages = 1; //if extraPages is allocated for either KH or UH, then max of one extra will be expected
		if (is_correct) eval += 15 ;
	}

	is_correct = 1;
	cprintf_colored(TEXT_cyan, "\n%~STEP B: checking the handles... [30%]\n");
	{
		if (LIST_SIZE(&(myEnv->shareHandles.share_handles_list)) != 3)
		{
			is_correct = 0;
			cprintf_colored(TEXT_TESTERR_CLR, "%~Share handles are not stored in the env\n");
		}
		struct ShareHandle *handle ;
		int numOfCorrectHandles = 0;
		for (int i = 0; i < SHARE_MAX_HANDLES; ++i)
		{
			if (myEnv->__shs__[i].name[0] == 'x') { numOfCorrectHandles++ ; }
			else if (myEnv->__shs__[i].name[0] == 'y') { numOfCorrectHandles++ ; }
			else if (myEnv->__shs__[i].name[0] == 'z') { numOfCorrectHandles++ ; }
		}
		if (numOfCorrectHandles != 3)
		{
			is_correct = 0;
			cprintf_colored(TEXT_TESTERR_CLR, "%~Share handles info is not stored correctly\n");
		}
	}
	if (is_correct) eval += 30 ;

	is_correct = 1;
	cprintf_colored(TEXT_cyan, "\n%~STEP C: checking reading & writing... [25%]\n");
	{
		int i=0;
		for(;i<PAGE_SIZE/4;i++)
		{
			x[i] = -1;
			y[i] = -1;
		}

		i=0;
		for(;i<2*PAGE_SIZE/4;i++)
		{
			z[i] = -1;
		}

		if( x[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( x[PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}

		if( y[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( y[PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}

		if( z[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( z[2*PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
	}
	if (is_correct)
		eval += 25 ;
	cprintf_colored(TEXT_light_green, "%~\n%~Test of Shared Variables [Create] [1] completed. Eval = %d%%\n\n", eval);

	return;
}
