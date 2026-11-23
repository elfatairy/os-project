/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 12 */
/* MAKE SURE Second_List_Size = 5 */
/* TEST THE LRU ALGO ON THE 2ND CHANCE LIST*/
/* *********************************************************** */

#include <inc/lib.h>

char __arr__[PAGE_SIZE*13];
char* __ptr__ = (char* )0x0803000 ;
char* __ptr2__ = (char* )0x0804000 ;
uint32 actual_active_list_init[7] = {0xeebfd000, 0x805000, 0x804000, 0x803000, 0x802000, 0x801000, 0x800000};
uint32 actual_second_list_init[5] = {0x80A000, 0x809000, 0x808000, 0x807000, 0x806000};
extern volatile bool printStats;

void _main(void)
{
	printStats = 0;
	int x = 0;

	//("STEP 0: checking Initial WS entries ...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list_init, actual_second_list_init, 7, 5);
		if(check == 0)
			panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}

	//Reading (Not Modified)
	char garbage1 = __arr__[PAGE_SIZE*11-1] ;
	char garbage2 = __arr__[PAGE_SIZE*12-1] ;
	char garbage4,garbage5;

	//Writing (Modified)
	int i ;
	for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
	{
		__arr__[i] = -1 ;
		/*2016: this BUGGY line is REMOVED el7! it overwrites the KERNEL CODE :( !!!*/
		//*__ptr__ = *__ptr2__ ;
		/*==========================================================================*/
		//always use pages at 0x803000 and 0x804000
		garbage4 = *__ptr__ + garbage5;
		garbage5 = *__ptr2__ + garbage4;
		__ptr__++ ; __ptr2__++ ;
	}

	//===================

	//("STEP 1: checking LRU LISTS after new page FAULTS...\n");
	uint32 actual_active_list[7] ;
	{
		actual_active_list[0] = 0x801000;
		actual_active_list[1] = 0x800000;
		actual_active_list[2] = 0x80c000;
		actual_active_list[3] = 0xeebfd000;
		actual_active_list[4] = 0x80b000;
		actual_active_list[5] = 0x804000;
		actual_active_list[6] = 0x803000;
	}
	uint32 actual_second_list[5] ;
	{
		actual_second_list[0] = 0x80a000 ;
		actual_second_list[1] = 0x809000 ;
		actual_second_list[2] = 0x808000 ;
		actual_second_list[3] = 0x807000 ;
		actual_second_list[4] = 0x806000 ;
	}

	bool is_correct = 1;

	int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 7, 5);
	if(check == 0)
		{ is_correct = 0; panic("PAGE LRU Lists entry checking failed when new PAGE FAULTs occurred..!!\n"); }

	//("STEP 2: Checking PAGE LRU LIST algorithm after faults due to ACCESS in the second chance list... \n");
	{
		uint32* secondlistVA = (uint32*)0x809000;
		x = x + *secondlistVA;
		secondlistVA = (uint32*)0x808000;
		x = x + *secondlistVA;
		secondlistVA = (uint32*)0x807000;
		x = x + *secondlistVA;

		{
			actual_active_list[0] = 0x807000;
			actual_active_list[1] = 0x808000;
			actual_active_list[2] = 0x809000;
			actual_active_list[3] = 0x801000;
			actual_active_list[4] = 0x800000;
			actual_active_list[5] = 0x80c000;
			actual_active_list[6] = 0xeebfd000;
		}
		{
			actual_second_list[0] = 0x80b000;
			actual_second_list[1] = 0x804000 ;
			actual_second_list[2] = 0x803000 ;
			actual_second_list[3] = 0x80a000 ;
			actual_second_list[4] = 0x806000 ;
		}

		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 7, 5);
		if(check == 0)
			{ is_correct = 0; panic("PAGE LRU Lists entry checking failed when a new PAGE ACCESS from the SECOND LIST is occurred..!!\n"); }
	}

	//("STEP 3: NEW FAULTS to test applying LRU algorithm on the second list by removing the LRU page... \n");
	{
		//Reading (Not Modified)
		char garbage3 = __arr__[PAGE_SIZE*13-1] ;
		{
			actual_active_list[0] = 0xeebfd000;
			actual_active_list[1] = 0x810000;
			actual_active_list[2] = 0x807000;
			actual_active_list[3] = 0x808000;
			actual_active_list[4] = 0x809000;
			actual_active_list[5] = 0x801000;
			actual_active_list[6] = 0x800000;
		}
		{
			actual_second_list[0] = 0x80c000;
			actual_second_list[1] = 0x80b000 ;
			actual_second_list[2] = 0x804000 ;
			actual_second_list[3] = 0x803000 ;
			actual_second_list[4] = 0x80a000 ;
		}

		check = sys_check_LRU_lists(actual_active_list, actual_second_list, 7, 5);
		if(check == 0)
			{ is_correct = 0; panic("PAGE LRU Lists entry checking failed when a new PAGE FAULT occurred..!!\n"); }
	}

	cprintf_colored(TEXT_light_green, "%~Congratulations... test PAGE replacement [LRU Alg. on the 2nd chance list] is completed successfully\n");
	return;
}
