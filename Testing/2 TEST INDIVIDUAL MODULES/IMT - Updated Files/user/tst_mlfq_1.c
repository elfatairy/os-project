#include <inc/lib.h>

void _main(void)
{
	// To check that the slave environments completed successfully
	rsttst();

	//Load and run small programs (expected to be finished first at level0)
	int ID;
	for (int i = 0; i < 4; ++i) {
		ID = sys_create_env("fib_small", (myEnv->page_WS_max_size), (myEnv->page_2nd_WS_size),(myEnv->percentage_of_WS_pages_to_be_removed));
		if (ID == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID);
	}

	//Wait a while to ensure that this env goes to the lowest level
	env_sleep(20000);

	//Load and run small programs (in case of FCFS: these programs should be finished after this program finishes its FCFS)
	for (int i = 0; i < 4; ++i) {
		ID = sys_create_env("fib_small", (myEnv->page_WS_max_size), (myEnv->page_2nd_WS_size),(myEnv->percentage_of_WS_pages_to_be_removed));
		if (ID == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID);
		//delay for a while to ensure that these 4 programs will be finished in order
		env_sleep(500);
	}

	//Load and run large programs (in case of FCFS: expected to be finished last at the lowest level BUT after this program)
	for (int i = 0; i < 4; ++i) {
		ID = sys_create_env("fib_large", (myEnv->page_WS_max_size), (myEnv->page_2nd_WS_size),(myEnv->percentage_of_WS_pages_to_be_removed));
		if (ID == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID);
		//delay for a while to ensure that these 4 programs will be finished in order
		env_sleep(5000);
	}

	env_sleep(80000);

	inctst();
}
