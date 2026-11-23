/*
 * ipc2_sender1.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>

extern volatile bool printStats;

void _main(void)
{
	printStats = 0;

	int ret, expected;
	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Connect to MainMB2 from the parent
	int handleMainMB2 = ipc_connect(myEnv->env_parent_id, "MainMB2", MB_SENDER);
	if (handleMainMB2 < 0) panic ("Failed to connect to MainMB2") ;

	//Acknowledge the parent
	inctst();
}

