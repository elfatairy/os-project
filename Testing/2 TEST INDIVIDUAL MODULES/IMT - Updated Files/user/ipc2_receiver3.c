/*
 * ipc2_receiver3.c
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

	//Connect to ALL mailboxes from the parent
	int handleMB1 = ipc_connect(myEnv->env_parent_id, "MB1", MB_RECEIVER);
	if (handleMB1 < 0) panic ("Failed to connect to MB1. ret = %d",handleMB1) ;
	int handleMB2 = ipc_connect(myEnv->env_parent_id, "MB2", MB_RECEIVER);
	if (handleMB2 < 0) panic ("Failed to connect to MB2. ret = %d",handleMB2) ;
	int handleMB3 = ipc_connect(myEnv->env_parent_id, "MB3", MB_RECEIVER);
	if (handleMB3 < 0) panic ("Failed to connect to MB3. ret = %d",handleMB3) ;

	//Acknowledge the master
	inctst();

	//Wait a signal from the master
	while (gettst() != 0) ;
}

