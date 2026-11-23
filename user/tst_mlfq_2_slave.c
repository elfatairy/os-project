
#include <inc/lib.h>

extern volatile bool printStats ;

int fibonacci(int n);

void
_main(void)
{
	printStats = 0;
	//cprintf("tst_mlfq_2_slave %d is running now\n", myEnv->env_id);
	//Recursively generate instances from this program
	int ID;
	ID = sys_create_env("tst_mlfq_2_slave", (myEnv->page_WS_max_size), (myEnv->page_2nd_WS_size),(myEnv->percentage_of_WS_pages_to_be_removed));
	if (ID == E_ENV_CREATION_ERROR)
		panic("RUNNING OUT OF ENV!! terminating...");
	sys_run_env(ID);

	int i1=24;
	int res = fibonacci(i1) ;

	//sys_lock_cons();
	{
		cprintf_colored(TEXT_light_magenta, "Finished\n");
		//cprintf_colored(TEXT_light_magenta, "\n>>>> Fibonacci #%d = %d <<<<\n",i1, res);
	}
	//sys_unlock_cons();

	if (res != 75025)
		panic("[envID %d] wrong result!", myEnv->env_id);

	//To indicate that it's completed successfully
	inctst();

	return;
}


int fibonacci(int n)
{
	if (n <= 1)
		return 1 ;
	return fibonacci(n-1) + fibonacci(n-2) ;
}

