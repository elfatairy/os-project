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

	for (int i = 3; i <= MB_MAX_HANDLES; ++i)
	{
		char prefix[NAMELEN] = "Main";
		char MBIndex[4];
		ltostr(i, MBIndex);
		char MBName[NAMELEN];
		strcconcat(prefix, MBIndex, MBName);
		int handle = sys_ipc_create_mailbox(MBName);
		if (handle < 0) {panic("%~Failed to create MB\n"); }
	}
	//Load & Run Validation Program
	int32 envIdValidation = sys_create_env("ipc2_validations", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	if (envIdValidation == E_ENV_CREATION_ERROR)
		panic("NO AVAILABLE ENVs...");
	sys_run_env(envIdValidation);

	//Wait for signal from "validations" to start loading & running ipc2_sender0
	while (gettst() != 1)
	{
		env_sleep(500);
	}

	//Load & run different instances from ipc2_sender0 that connect to MainMB2
	for (int i = 0; i < MB_MAX_SENDERS; ++i)
	{
		int32 envIdSender = sys_create_env("ipc2_sender0", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdSender == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdSender);
	}
}

