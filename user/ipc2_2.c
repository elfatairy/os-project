/*
 * ipc2_2.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>
#define NUM_OF_SENDERS 10
#define NUM_OF_RECEIVERS 20

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
	cprintf_colored(TEXT_cyan, "%~\n1. Create Mailboxes\n") ;

	char MBforAck[NAMELEN] = "MBforAck" ;
	char MBforAll[NAMELEN] = "MBforAll" ;
	char MBforEven[NAMELEN] = "MBforEven" ;
	char MBforOdd[NAMELEN] = "MBforOdd" ;

	int handleMBforAck = sys_ipc_create_mailbox(MBforAck);
	if (handleMBforAck < 0) {panic("%~Failed to create MBforAck\n"); }
	int handleMBforAll = sys_ipc_create_mailbox(MBforAll);
	if (handleMBforAll < 0) {panic("%~Failed to create MBforAll\n"); }
	int handleMBforEven = sys_ipc_create_mailbox(MBforEven);
	if (handleMBforEven < 0) {panic("%~Failed to create MBforEven\n"); }
	int handleMBforOdd = sys_ipc_create_mailbox(MBforOdd);
	if (handleMBforOdd < 0) {panic("%~Failed to create MBforOdd\n"); }

	//Load & Run Senders & Receivers
	cprintf_colored(TEXT_cyan, "%~\n2. Load & Run Senders & Receivers\n") ;

	int32 envIdSenders[NUM_OF_SENDERS];
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		envIdSenders[i] = sys_create_env("ipc2_sender1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdSenders[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdSenders[i]);
	}
	int32 envIdReceivers[NUM_OF_RECEIVERS];
	for (int i = 0; i < NUM_OF_RECEIVERS; ++i)
	{
		envIdReceivers[i] = sys_create_env("ipc2_receiver1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdReceivers[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdReceivers[i]);
	}

	//Wait until ALL senders & receivers connected to the mailboxes
	while (gettst() != NUM_OF_SENDERS + NUM_OF_RECEIVERS);

	//Check the senders/receivers array inside the three mailboxes
	cprintf_colored(TEXT_cyan, "%~\n3. Check the senders/receivers array inside the three mailboxes\n") ;

	int found ;
	int expectedSenders[NUM_OF_SENDERS] ;
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		expectedSenders[i] = envIdSenders[i];
	}
	int expectedReceivers[NUM_OF_RECEIVERS] ;
	for (int i = 0; i < NUM_OF_RECEIVERS; ++i)
	{
		expectedReceivers[i] = envIdReceivers[i];
	}
	char cmd1[NAMELEN*2]  ;
	char finalCmd[NAMELEN*2]  ;
	char separator[2] = "@";
	char checkMBCmd[100] = "__CheckMBConnections@";
	char token[20] ;
	ltostr(NUM_OF_SENDERS, token); strcconcat(checkMBCmd, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr(NUM_OF_RECEIVERS, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedSenders, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedReceivers, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	{
		strcconcat(cmd1, MBforAll, finalCmd);
		//atomic_cprintf("%~Ref Command = %s\n", finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforAll);
	}
	{
		strcconcat(cmd1, MBforEven, finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforEven);
	}
	{
		strcconcat(cmd1, MBforOdd, finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforOdd);
	}

	//Acknowledge the Senders & Receivers to continue their work
	cprintf_colored(TEXT_cyan, "%~\n4. Acknowledge the Senders & Receivers to continue their work\n") ;
	{
		rsttst();
		cprintf_colored(TEXT_cyan, "%~\n5. Acknowledge the Senders to continue their work\n") ;
		char ackCmd[NAMELEN] = "__StopOtherProcesses__";
		sys_utilities(ackCmd, 0);
		//	struct Message msg ;
		//	msg.type = MSG_NULL;
		//	msg.senderID = envID;
		//	msg.receiverID = UINT_MAX;
		//	{
		//		ipc_broadcast(handleMBforAck, &msg);
		//	}
	}

	//Wait until ALL senders & receivers disconnected from the even/odd mailboxes
	while (gettst() != NUM_OF_SENDERS + NUM_OF_RECEIVERS);

	//Check the senders/receivers array inside the three mailboxes
	cprintf_colored(TEXT_cyan, "%~\n6. Check the senders/receivers array inside the three mailboxes\n") ;
	{
		strcconcat(cmd1, MBforAll, finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforAll);
	}
	int expectedEvenSenders[NUM_OF_SENDERS] ; int numOfEvenSenders = 0;
	int expectedOddSenders[NUM_OF_SENDERS] ; int numOfOddSenders = 0;
	int expectedEvenReceivers[NUM_OF_RECEIVERS] ; int numOfEvenReceivers = 0;
	int expectedOddReceivers[NUM_OF_RECEIVERS] ; int numOfOddReceivers = 0;
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		if (envIdSenders[i] % 2 == 0)
			expectedEvenSenders[numOfEvenSenders++] = envIdSenders[i];
		else
			expectedOddSenders[numOfOddSenders++] = envIdSenders[i];
	}
	for (int i = 0; i < NUM_OF_RECEIVERS; ++i)
	{
		if (envIdReceivers[i] % 2 == 0)
			expectedEvenReceivers[numOfEvenReceivers++] = envIdReceivers[i];
		else
			expectedOddReceivers[numOfOddReceivers++] = envIdReceivers[i];
	}
	ltostr(numOfEvenSenders, token); strcconcat(checkMBCmd, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr(numOfEvenReceivers, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedEvenSenders, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedEvenReceivers, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	{
		strcconcat(cmd1, MBforEven, finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforEven);
	}
	ltostr(numOfOddSenders, token); strcconcat(checkMBCmd, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr(numOfOddReceivers, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedOddSenders, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	ltostr((uint32)&expectedOddReceivers, token); strcconcat(cmd1, token, cmd1); strcconcat(cmd1, separator, cmd1);
	{
		strcconcat(cmd1, MBforOdd, finalCmd);
		sys_utilities(finalCmd, (uint32)&found);
		if (found != 1) panic("Connection failed! senders/receivers array of %s MB is not set correctly", MBforOdd);
	}

	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#2 test connection/disconnection completed successfully!!\n\n\n");

}

