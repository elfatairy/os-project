/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 20 */
/* MAKE SURE Second_List_Size = 2 */
/*FIRST SCENARIO --> ACTIVE LIST NOT FULL*/
/* *********************************************************** */

#include <inc/lib.h>
uint8 __tmpArr__[PAGE_SIZE * 6] = {0};
extern volatile bool printStats;

void _main(void)
{
	printStats = 0;

	int freePages = sys_calculate_free_frames();

	//	panic("envID = %d\n",envID);
	char arr[PAGE_SIZE*1024*4];

	uint32 actual_active_list[17] ;
	{
		actual_active_list[0] = 0xedbfd000;
		actual_active_list[1] = 0xeebfd000;
		actual_active_list[2] = 0x809000;
		actual_active_list[3] = 0x808000;
		actual_active_list[4] = 0x807000;
		actual_active_list[5] = 0x806000;
		actual_active_list[6] = 0x805000;
		actual_active_list[7] = 0x804000;
		actual_active_list[8] = 0x803000;
		actual_active_list[9] = 0x802000;
		actual_active_list[10] = 0x801000;
		actual_active_list[11] = 0x800000;
	}
	uint32 actual_second_list[2] = {};

	("STEP 0: checking Initial LRU lists entries ...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 12, 0);
		if(check == 0)
			panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}

	int usedDiskPages = sys_pf_calculate_allocated_pages() ;
	int i=0;
	for(;i<=PAGE_SIZE;i++)
	{
		arr[i] = 'A';
	}
//	panic("1. free frames = %d\n", sys_calculate_free_frames());

	i=PAGE_SIZE*1024;
	for(;i<=(PAGE_SIZE*1024 + PAGE_SIZE);i++)
	{
		arr[i] = 'A';
	}
	//panic("2. free frames = %d\n", sys_calculate_free_frames());

	i=PAGE_SIZE*1024*2;
	for(;i<=(PAGE_SIZE*1024*2 + PAGE_SIZE);i++)
	{
		arr[i] = 'A';
	}
	//panic("3. free frames = %d\n", sys_calculate_free_frames());


	bool is_correct = 1;
	uint32 expected, actual ;
	cprintf_colored(TEXT_light_cyan, "%~STEP A: checking PLACEMENT fault handling ... \n");
	{
		if( arr[0] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n");}
		if( arr[PAGE_SIZE] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n");}

		if( arr[PAGE_SIZE*1024] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n"); }
		if( arr[PAGE_SIZE*1025] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n"); }

		if( arr[PAGE_SIZE*1024*2] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n"); }
		if( arr[PAGE_SIZE*1024*2 + PAGE_SIZE] != 'A')  { is_correct = 0; panic("PLACEMENT of stack page failed\n"); }

		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) { is_correct = 0; panic("new stack pages should NOT written to Page File until it's replaced\n"); }

		expected = 6 /*pages*/ + 3 /*tables*/ - 2 /*table + page due to a fault in the 1st call of sys_calculate_free_frames*/;
		actual = (freePages - sys_calculate_free_frames()) ;
		//actual = (initFreeFrames - sys_calculate_free_frames()) ;

		if(actual != expected) { is_correct = 0; panic("allocated memory size incorrect. Expected = %d, Actual = %d\n", expected, actual); }
	}
	cprintf_colored(TEXT_light_cyan, "%~STEP A passed: PLACEMENT fault handling works!\n\n");

	for (int i=16;i>4;i--)
		actual_active_list[i]=actual_active_list[i-5];

	actual_active_list[0]=0xee3fe000;
	actual_active_list[1]=0xee3fd000;
	actual_active_list[2]=0xedffe000;
	actual_active_list[3]=0xedffd000;
	actual_active_list[4]=0xedbfe000;

	cprintf_colored(TEXT_light_cyan, "%~STEP B: checking LRU lists entries ...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 17, 0);
			if(check == 0)
				{ is_correct = 0; panic("LRU lists entries are not correct, check your logic again!!\n"); }
	}
	cprintf_colored(TEXT_light_cyan, "%~STEP B passed: LRU lists entries test are correct\n\n");

	cprintf_colored(TEXT_light_green, "%~\nCongratulations!! Test of PAGE PLACEMENT FIRST SCENARIO completed successfully\n");

	return;
}
