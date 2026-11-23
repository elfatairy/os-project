/*
 * ipc2_1.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>

extern volatile bool printStats;

void _main(void)
{
	printStats = 0;

	rsttst();
	int ret;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Create Mailboxes
	int handle1 = sys_ipc_create_mailbox("MainMB1");
	if (handle1 < 0) {panic("%~Failed to create MB\n"); }
	int handle2 = sys_ipc_create_mailbox("MainMB2");
	if (handle2 < 0) {panic("%~Failed to create MB\n"); }

	//Load & Run Validation Program
	int32 envIdValidation = sys_create_env("ipc2_0_slave", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	if (envIdValidation == E_ENV_CREATION_ERROR)
		panic("NO AVAILABLE ENVs...");
	sys_run_env(envIdValidation);
}

