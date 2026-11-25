/*
 * ipc2_4.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>
#define NUM_OF_SENDERS 5
#define NUM_OF_RECEIVERS 15
#define NUM_OF_SKIPPED_RECEIVERS 5

extern volatile bool printStats;

void _main(void)
{
	printStats = 0;
	rsttst();
	int ret;
	struct Message msg;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Create Mailboxes
	cprintf_colored(TEXT_cyan, "%~\n1. Create Mailboxes\n") ;

	char MBforAck[NAMELEN] = "MBforAck" ;
	char MB1[NAMELEN] = "MB1" ;
	char MB2[NAMELEN] = "MB2" ;
	char MB3[NAMELEN] = "MB3" ;

	int handleMBforAck = sys_ipc_create_mailbox(MBforAck);
	if (handleMBforAck < 0) {panic("%~Failed to create MBforAck\n"); }
	int handleMB1 = sys_ipc_create_mailbox(MB1);
	if (handleMB1 < 0) {panic("%~Failed to create MB1\n"); }
	int handleMB2 = sys_ipc_create_mailbox(MB2);
	if (handleMB2 < 0) {panic("%~Failed to create MB2\n"); }
	int handleMB3 = sys_ipc_create_mailbox(MB3);
	if (handleMB3 < 0) {panic("%~Failed to create MB3\n"); }

	//Load & Run Senders & Receivers
	cprintf_colored(TEXT_cyan, "%~\n2. Load & Run Senders & Receivers\n") ;

	int32 envIdSenders[NUM_OF_SENDERS];
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		envIdSenders[i] = sys_create_env("ipc2_sender2", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdSenders[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdSenders[i]);
	}
	int32 envIdSkippedReceivers[NUM_OF_SKIPPED_RECEIVERS];
	for (int i = 0; i < NUM_OF_SKIPPED_RECEIVERS; ++i)
	{
		envIdSkippedReceivers[i] = sys_create_env("ipc2_receiver3", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdSkippedReceivers[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdSkippedReceivers[i]);
	}
	int32 envIdReceivers[NUM_OF_RECEIVERS];
	for (int i = 0; i < NUM_OF_RECEIVERS; ++i)
	{
		envIdReceivers[i] = sys_create_env("ipc2_receiver2", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdReceivers[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
		sys_run_env(envIdReceivers[i]);
	}
	//Wait until ALL senders & receivers connected to the mailboxes
	while (gettst() != NUM_OF_SENDERS + NUM_OF_RECEIVERS + NUM_OF_SKIPPED_RECEIVERS);

	//Fill-up the Local Mailbox of Some Receivers
	cprintf_colored(TEXT_cyan, "%~\n3. Fill-up the Local Mailbox of Some Receivers\n") ;
	{
		msg.type = MSG_NULL;
		msg.senderID = envID;
		msg.receiverID = UINT_MAX;
		for (int i = 0; i < MB_MAX_MESSAGES; ++i)
		{
			ipc_broadcast(handleMB3, &msg);
		}
	}

	//Acknowledge the Senders to continue their work
	cprintf_colored(TEXT_cyan, "%~\n4. Acknowledge the Senders to continue their work\n") ;
	char ackCmd[NAMELEN] = "__StopOtherProcesses__";
	sys_utilities(ackCmd, 0);

//	struct Message msg ;
//	msg.type = MSG_NULL;
//	msg.senderID = envID;
//	msg.receiverID = UINT_MAX;
//	{
//		ipc_broadcast(handleMBforAck, &msg);
//	}

	//Wait until ALL senders broadcast their messages
	while (gettst() != 2*NUM_OF_SENDERS + NUM_OF_RECEIVERS + NUM_OF_SKIPPED_RECEIVERS);

	//Check the local mailbox of all receivers
	cprintf_colored(TEXT_cyan, "%~\n4. Check the local mailbox of all receivers\n") ;
	{
		char cmd1[NAMELEN*2]  ;
		char finalCmd[NAMELEN*2]  ;
		char separator[2] = "@";
		char checkMBCmd[NAMELEN] = "__CheckLocalMBMsgs@";
		char token[20] ;
		ltostr(NUM_OF_SENDERS, token);strcconcat(checkMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		int expectedCopyPerMsg = 2;
		ltostr(expectedCopyPerMsg, token);strcconcat(cmd1, token, cmd1);strcconcat(cmd1, separator, cmd1);
		int expectedMsgType = MSG_DATA;
		ltostr(expectedMsgType, token);strcconcat(cmd1, token, cmd1);strcconcat(cmd1, separator, cmd1);
		//Concatenate expected messages (IDs of all senders)
		for (int i = 0; i < NUM_OF_SENDERS; ++i)
		{
			ltostr(envIdSenders[i], token);
			strcconcat(cmd1, token, cmd1);
			strcconcat(cmd1, separator, cmd1);
		}
		//Check local mailbox of each receiver
		for (int i = 0; i < NUM_OF_RECEIVERS; ++i)
		{
			bool found = 0;
			ltostr(envIdReceivers[i], token);
			strcconcat(cmd1, token, finalCmd);
			//cprintf("finalCmd is: %s\n", finalCmd);
			sys_utilities(finalCmd, (uint32)&found);
			if (found != 1) panic("Broadcast failed! receiver %d doesn't get all messages", envIdReceivers[i]);
		}
		//Check local mailbox of skipped receivers
		{
			ltostr(MB_MAX_MESSAGES, token);strcconcat(checkMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
			expectedCopyPerMsg = 1;
			ltostr(expectedCopyPerMsg, token);strcconcat(cmd1, token, cmd1);strcconcat(cmd1, separator, cmd1);
			expectedMsgType = MSG_NULL;
			ltostr(expectedMsgType, token);strcconcat(cmd1, token, cmd1);strcconcat(cmd1, separator, cmd1);

			for (int i = 0; i < NUM_OF_SKIPPED_RECEIVERS; ++i)
			{
				bool found = 0;
				ltostr(envIdSkippedReceivers[i], token);
				strcconcat(cmd1, token, finalCmd);
				//cprintf("finalCmd is: %s\n", finalCmd);
				sys_utilities(finalCmd, (uint32)&found);
				if (found != 1) panic("Broadcast failed! receiver %d doesn't get all NULL messages", envIdReceivers[i]);
			}
		}
	}

	//Acknowledge all receivers to finish
	rsttst();

	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#2 test M-N broadcast messages with skipped receivers completed successfully!!\n\n\n");

}

