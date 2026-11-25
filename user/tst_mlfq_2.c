#include <inc/lib.h>

void _main(void)
{
	// To check that the slave environments completed successfully
	rsttst();

	//wait long time to ensure that this program goes to the lower level
	cprintf("\nLoading... please wait!\n");
	env_sleep(20000);

	//generate ONE instance from the slave program
	int ID;
	ID = sys_create_env("tst_mlfq_2_slave", (myEnv->page_WS_max_size), (myEnv->page_2nd_WS_size),(myEnv->percentage_of_WS_pages_to_be_removed));
	if (ID == E_ENV_CREATION_ERROR)
		panic("RUNNING OUT OF ENV!! terminating...");
	sys_run_env(ID);

	//Ensure that at least 30 instances of the slave program are successfully finished before killing all remaining slaves
	while (gettst() < 30) ;

//	char cmd1[NAMELEN] = "__StopOtherProcesses__" ;
//	sys_utilities(cmd1, 0);

	//Exit all remaining slaves
	char cmd2[NAMELEN] = "__ExitAllReadyEnvs__" ;
	sys_utilities(cmd2, 0);

	inctst();
}
