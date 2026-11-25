#include <kern/proc/priority_manager.h>
#include <inc/assert.h>
#include <kern/proc/user_environment.h>
#include <kern/cmd/command_prompt.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/cpu/sched.h>
#include "../mem/memory_manager.h"


#define INSTANCES_NUMBER 4
#define TOTAL_TEST_VALUES 3
uint8 firstTimeTest = 1;
int prog_orders[TOTAL_TEST_VALUES+10][INSTANCES_NUMBER] = {0};
int env_count[TOTAL_TEST_VALUES+10] = {0};

void print_order(int prog_orders[][INSTANCES_NUMBER])
{
	for (int i = 0; i < TOTAL_TEST_VALUES; i++)
	{
		cprintf("\t[%d]: ", i);
		for (int j = 0; j < env_count[i]; j++)
		{
			if (prog_orders[i][j] == 0)
				break;
			cprintf("%d, ", prog_orders[i][j]);
		}
		cprintf("\n");
	}
}

int find_in_range(int env_id, int start, int count)
{
	int ret = -1;
	acquire_kspinlock(&ProcessQueues.qlock);
	{
		struct Env *env = NULL;
		int i = 0, end = start + count;

		//REVERSE LOOP ON EXIT LIST (to be the same as the queue order)
		int numOfExitEnvs = LIST_SIZE(&ProcessQueues.env_exit_queue);
		env = LIST_LAST(&ProcessQueues.env_exit_queue);

		cprintf("searching for envID %d starting from %d till %d\n", env_id, start, end);
		for (; i < numOfExitEnvs; env = LIST_PREV(env))
			//LIST_FOREACH_R(env, &env_exit_queue)
		{
			if (i < start)
			{
				i++;
				continue;
			}
			if (i >= end)
				//return -1;
				break;

			if (env_id == env->env_id)
			{
				ret = i;
				break;
			}
			i++;
		}
	}
	release_kspinlock(&ProcessQueues.qlock);
	return ret;
}


void test_bsd_nice_0()
{
}

void test_bsd_nice_1()
{
}

void test_bsd_nice_2()
{
}


void test_priorityRR_0()
{
}

void test_priorityRR_1()
{
}

void test_priorityRR_2()
{
}


void test_MLFQ_0()
{
	/* 4 CPUs
	 * FOS> schedMLFQ 3 10 50 100 1000000000
	 * FOS> tst MLFQ 0
	 * FOS> tst MLFQ 0
	*/
	int numOfIncorrect = 0;
	int testNum ;
	if (firstTimeTest)
	{
		firstTimeTest = 0;
		//[1] Load Large Programs (fib_large)
		testNum = 2;
		for (int i = 0; i < INSTANCES_NUMBER; i++)
		{
			struct Env *env ;
			{
				env = env_create("fib_large", 500, 0, 0);
				if (env == NULL)
					panic("Loading programs failed\n");
				if (env->page_WS_max_size != 500)
					panic("The program working set size is not correct\n");
				sched_new_env(env);
			}
			prog_orders[testNum][env_count[testNum]++] = env->env_id;
		}
		//[2] Load Medium Programs (fib_medium)
		testNum = 1;
		for (int i = 0; i < INSTANCES_NUMBER; i++)
		{
			struct Env *env ;
			{
				env = env_create("fib_medium", 500, 0, 0);
				if (env == NULL)
					panic("Loading programs failed\n");
				if (env->page_WS_max_size != 500)
					panic("The program working set size is not correct\n");
				sched_new_env(env);
			}
			prog_orders[testNum][env_count[testNum]++] = env->env_id;
		}
		//[3] Load Small Programs (fib_small)
		testNum = 0;
		for (int i = 0; i < INSTANCES_NUMBER; i++)
		{
			struct Env *env ;
			{
				env = env_create("fib_small", 500, 0, 0);
				if (env == NULL)
					panic("Loading programs failed\n");
				if (env->page_WS_max_size != 500)
					panic("The program working set size is not correct\n");
				sched_new_env(env);
			}
			prog_orders[testNum][env_count[testNum]++] = env->env_id;
		}

		print_order(prog_orders);
		cprintf_colored(TEXT_light_cyan, "\n> Running... (After all running programs finish, Run the same command again.)\n");
		execute_command("runall");
	}
	else
	{
		cprintf_colored(TEXT_light_cyan, "\n> Checking...\n");
		sched_print_all();
		// print_order(prog_orders);
		int start_idx = 0;
		for (int i = 0; i < TOTAL_TEST_VALUES; i++)
		{
			int expectedLevel = i;
			for (int j = 0; j < INSTANCES_NUMBER; j++)
			{
				int exist = find_in_range(prog_orders[i][j], start_idx, env_count[i]);
				if (exist == -1)
				{
					cprintf_colored(TEXT_TESTERR_CLR, "The finish order of program [%d] is not correct\n", prog_orders[i][j]);
					numOfIncorrect++;
				}
				struct Env* e = NULL;
				envid2env(prog_orders[i][j], &e, 0);
				if (exist != -1 && e->isExpandedBefore != expectedLevel)
				{
					cprintf_colored(TEXT_TESTERR_CLR, "The finish queue of program [%d] is not correct\n", prog_orders[i][j]);
					numOfIncorrect++;
				}
			}
			start_idx += env_count[i];
		}
		firstTimeTest = 0;
	}
	if (gettst() != 12)
		panic("one or more environment are not finished successfully!");

	int eval = 100 - numOfIncorrect * 100 / (TOTAL_TEST_VALUES * INSTANCES_NUMBER);
	cprintf_colored(TEXT_light_green, "\ntest_MLFQ_0 is finished. Eval = %d%\n", eval);
}

void test_MLFQ_1()
{
	/* 1 CPU
	 * 		FOS> schedMLFQ 5 10 10 10 10 0 1000000000
	 * 		FOS> tst MLFQ 1
	 * 		FOS> tst MLFQ 1
	*/
	int numOfIncorrect = 0;
	int testNum ;
	if (firstTimeTest)
	{
		int firstID ;
		firstTimeTest = 0;
		//[1] Load & run main program
		struct Env *env ;
		{
			env = env_create("tst_mlfq_1", 500, 0, 0);
			if (env == NULL)
				panic("Loading programs failed\n");
			if (env->page_WS_max_size != 500)
				panic("The program working set size is not correct\n");
			sched_new_env(env);
			firstID = env->env_id;
		}
		//[2] Set expected finish order
		env_count[0] = 4;
		for (int i = 1; i <= env_count[0]; ++i)
		{
			prog_orders[0][i-1] = firstID + i;
		}

		//FCFS Case
		if (quantums[4] == 0)
		{
			env_count[1] = 1;
			prog_orders[1][0] = firstID ;

			env_count[2] = 4;
			for (int i = 1; i <= env_count[2]; ++i)
			{
				prog_orders[2][i-1] = firstID + 4 + i;
			}

			env_count[3] = 4;
			for (int i = 1; i <= env_count[3]; ++i)
			{
				prog_orders[3][i-1] = firstID + 8 + i;
			}
		}
		//RR Case
		else
		{
			env_count[1] = 4;
			for (int i = 1; i <= env_count[1]; ++i)
			{
				prog_orders[1][i-1] = firstID + 4 + i;
			}

			env_count[2] = 4;
			for (int i = 1; i <= env_count[2]; ++i)
			{
				prog_orders[2][i-1] = firstID + 8 + i;
			}

			env_count[3] = 1;
			prog_orders[3][0] = firstID ;
		}
		print_order(prog_orders);
		cprintf_colored(TEXT_light_cyan, "\n> Running... (After all running programs finish, Run the same command again.)\n");
		execute_command("runall");
	}
	else
	{
		cprintf_colored(TEXT_light_cyan, "\n> Checking...\n");
		sched_print_all();
		// print_order(prog_orders);
		int start_idx = 0;
		int expectedLevel ;
		for (int i = 0; i < TOTAL_TEST_VALUES + 1; i++)
		{
			if (i == 0 || (quantums[4] == 0 && i == 2) || (quantums[4] != 0 && i == 1)) expectedLevel = 0;
			else expectedLevel = 4;
			for (int j = 0; j < env_count[i]; j++)
			{
				int exist = find_in_range(prog_orders[i][j], start_idx, 1);
				if (exist == -1)
				{
					cprintf_colored(TEXT_TESTERR_CLR, "The finish order of program [%d] is not correct\n", prog_orders[i][j]);
					numOfIncorrect++;
				}
				struct Env* e = NULL;
				envid2env(prog_orders[i][j], &e, 0);
				if (exist != -1 && e->isExpandedBefore != expectedLevel)
				{
					cprintf_colored(TEXT_TESTERR_CLR, "The finish queue of program [%d - %s] is not correct\n", prog_orders[i][j], e->prog_name);
					numOfIncorrect++;
				}
				start_idx++ ;
			}
		}
		firstTimeTest = 0;
	}

	if (gettst() != 13)
		panic("one or more environment are not finished successfully!");

	if (numOfIncorrect == 0)
		cprintf_colored(TEXT_light_green, "\nCongratulations... test_MLFQ_1 is finished successfully.\n");
}

void test_MLFQ_2()
{
	/* 1 CPU
	 * FOS> schedMLFQ 4 10 20 30 40 100000
	 * FOS> tst MLFQ 2
	 * FOS> tst MLFQ 2
	*/

	int numOfIncorrect = 0;
	int expectedNumOfFinishedSlaves = 30;
	int testNum ;
	if (firstTimeTest)
	{
		int firstID ;
		firstTimeTest = 0;
		//[1] Load & run main program
		struct Env *env ;
		{
			env = env_create("tst_mlfq_2", 500, 0, 0);
			if (env == NULL)
				panic("Loading programs failed\n");
			if (env->page_WS_max_size != 500)
				panic("The program working set size is not correct\n");
			sched_new_env(env);
			firstID = env->env_id;
		}
		//[2] Set expected finish order
		env_count[0] = 1;
		prog_orders[0][0] = firstID ;

		cprintf_colored(TEXT_light_cyan, "\n> Running... (After all running programs finish, Run the same command again.)\n");
		execute_command("runall");
	}
	else
	{
		cprintf_colored(TEXT_light_cyan, "\n> Checking...\n");
		sched_print_all();
		// print_order(prog_orders);

		//Make sure that the first N slave programs are finished first
		int start_idx = 0;
		int expectedLevel = 0 ;
		int firstID = prog_orders[0][0] ;
		for (int id = firstID+1; id <= firstID + expectedNumOfFinishedSlaves; id++)
		{
			int exist = find_in_range(id, start_idx, expectedNumOfFinishedSlaves);
			if (exist == -1)
			{
				cprintf_colored(TEXT_TESTERR_CLR, "The finish order of program [%d] is not correct\n", id);
				numOfIncorrect++;
			}
			struct Env* e = NULL;
			envid2env(id, &e, 0);
			if (exist != -1 && e->isExpandedBefore != expectedLevel)
			{
				cprintf_colored(TEXT_TESTERR_CLR, "The finish queue of program [%d - %s] is not correct\n", id, e->prog_name);
				numOfIncorrect++;
			}
		}

		//Make sure that the main program is finished last
		int last_idx = LIST_SIZE(&ProcessQueues.env_exit_queue);
		int exist = find_in_range(firstID, last_idx - 1, 1);
		if (exist == -1)
		{
			cprintf_colored(TEXT_TESTERR_CLR, "The finish order of program [%d] is not correct\n", firstID);
			numOfIncorrect += expectedNumOfFinishedSlaves / 2; //half grades depend on correctly finishing the main env
		}
		struct Env* e = NULL;
		envid2env(firstID, &e, 0);
		if (exist != -1 && e->isExpandedBefore != expectedLevel)
		{
			cprintf_colored(TEXT_TESTERR_CLR, "The finish queue of program [%d - %s] is not correct\n", firstID, e->prog_name);
			numOfIncorrect += expectedNumOfFinishedSlaves / 2; //half grades depend on correctly finishing the main env
		}
		firstTimeTest = 0;
	}

	if (gettst() < expectedNumOfFinishedSlaves + 1)
		panic("one or more environment are not finished successfully!");

	int eval = MAX(0, 100 - numOfIncorrect * 100 / (expectedNumOfFinishedSlaves));
	cprintf_colored(TEXT_light_green, "\ntest_MLFQ_2 is finished. Eval = %d%\n", eval);
}
