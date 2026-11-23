/*
 * ipc_sender1.c
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

	uint8 is_correct ;
	uint8 eval = 0;
	int handle0, handle1, handle2, handle3;

	//1. Validation of Create
	cprintf_colored(TEXT_cyan, "%~\n1. Validation of Create #1\n") ;
	is_correct = 1;
	{
		cprintf_colored(TEXT_light_cyan, "%~\na. Create MB with same name but in different env\n") ;
		handle0 = sys_ipc_create_mailbox("MainMB1");
		if (handle0 < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to create MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\nb. Create MB with different name\n") ;
		handle1 = sys_ipc_create_mailbox("MB1");
		if (handle1 < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to create MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\nc. Create MB (already exist)\n") ;
		ret = sys_ipc_create_mailbox("MB1");
		if (ret != E_MB_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to create an existing MB\n"); }
	}
	eval += is_correct * 10;

	//2. Validation of Connect
	cprintf_colored(TEXT_cyan, "%~\n2. Validation of Connect\n") ;
	is_correct = 1;
	{
		cprintf_colored(TEXT_light_cyan, "%~\n	a. MB not exists\n") ;
		ret = sys_ipc_connect(myEnv->env_parent_id, "NotExist", MB_SENDER);
		if (ret != E_MB_NOT_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to connect to a non-existing MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Already owned MB\n") ;
		ret = sys_ipc_connect(envID, "MB1", MB_SENDER);
		if (ret != E_MB_INV_REQUEST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to connect to an owned MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	c. Env already connected\n") ;
		handle2 = sys_ipc_connect(myEnv->env_parent_id, "MainMB1", MB_SENDER);
		if (handle2 < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to connect to MB\n"); }
		ret = sys_ipc_connect(myEnv->env_parent_id, "MainMB1", MB_RECEIVER);
		if (ret != E_MB_ENV_CONNECTED) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to connect to an already connected MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	d. Senders/Receivers Array is FULL\n") ;
		//tell the parent to load and run many instances from ipc2_sender0
		inctst();

		//Wait until all senders connect to MainMB2
		while (gettst() != MB_MAX_SENDERS + 1)
		{
			env_sleep(500);
		}

		//Attempt to connect to a MB with full senders array
		ret = ipc_connect(myEnv->env_parent_id, "MainMB2", MB_SENDER);
		if (ret != E_MB_OPER_FAILED) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to connect to a MB with FULL senders list\n"); }
	}
	eval += is_correct * 20;

	//3. Validation of Disconnect
	cprintf_colored(TEXT_cyan, "%~\n3. Validation of Disconnect\n") ;
	is_correct = 1;
	{
		cprintf_colored(TEXT_light_cyan, "%~\n	a. MB not exists\n") ;
		int nonExistHandle = handle2 + 1;
		ret = sys_ipc_disconnect(nonExistHandle);
		if (ret != E_MB_NOT_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to disconnect from a non-existing MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Already owned MB\n") ;
		ret = sys_ipc_disconnect(handle1);
		if (ret != E_MB_INV_REQUEST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to disconnect from an owned MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	c. Env is not registered in MB\n") ;
		ret = sys_ipc_disconnect(handle2);
		if (ret < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to disconnect from MB\n"); }
		ret = sys_ipc_disconnect(handle2);
		if (ret != E_MB_NOT_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to disconnect from a non-registered MB\n"); }
	}
	eval += is_correct * 15;

	//4. Validation of Delete
	cprintf_colored(TEXT_cyan, "%~\n4. Validation of Delete\n") ;
	is_correct = 1;
	{
		cprintf_colored(TEXT_light_cyan, "%~\n	a. Not owned MB\n") ;
		handle3 = sys_ipc_connect(myEnv->env_parent_id, "MainMB2", MB_RECEIVER);
		if (handle3 < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to connect to MB\n"); }
		ret = sys_ipc_delete_mailbox(handle3);
		if (ret != E_MB_INV_OWNER) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to delete a not-owned MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Non-exist MB\n") ;
		int nonExistHandle = handle1 + 1;
		ret = sys_ipc_delete_mailbox(nonExistHandle);
		if (ret != E_MB_NOT_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to delete a non-existing MB\n"); }
	}
	eval += is_correct * 15;

	//5. Validation of Broadcast
	cprintf_colored(TEXT_cyan, "%~\n5. Validation of Broadcast\n") ;
	is_correct = 1;
	{
		char msgBody[NAMELEN] = "hello";
		struct Message msg ;
		ret = ipc_create_data_message((uint8*)msgBody, strlen(msgBody), &msg);
		if (ret != 0)
			panic("Failed to create the message");
		msg.senderID = envID;
		msg.receiverID = UINT_MAX;

		cprintf_colored(TEXT_light_cyan, "%~\n	a. Not connected\n") ;
		ret = sys_ipc_broadcast(handle2, &msg);
		if (ret != E_MB_INV_REQUEST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast to a not-connected MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Connected as a receiver\n") ;
		ret = sys_ipc_broadcast(handle3, &msg);
		if (ret != E_MB_INV_REQUEST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast to a receiver MB\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	c. MB not exists\n") ;
		int nonExistHandle = handle1 + 1;
		ret = sys_ipc_broadcast(nonExistHandle, &msg);
		if (ret != E_MB_NOT_EXIST) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast to a non-existing MB\n"); }
	}
	eval += is_correct * 20;

	//6. Validation of Broadcast Message
	cprintf_colored(TEXT_cyan, "%~\n6. Validation of Broadcast Message\n") ;
	is_correct = 1;
	{
		char msgBody[NAMELEN] = "hello";
		struct Message msg ;
		ret = ipc_create_data_message((uint8*)msgBody, strlen(msgBody), &msg);
		if (ret != 0)
			panic("Failed to create the message");
		msg.senderID = envID;
		msg.receiverID = UINT_MAX;
		ret = ipc_broadcast(handle1, &msg);
		if (ret < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to broadcast a message\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	a. Invalid IDs\n") ;
		msg.senderID = myEnv->env_parent_id;
		msg.receiverID = UINT_MAX;
		ret = ipc_broadcast(handle1, &msg);
		if (ret != E_MB_OPER_FAILED) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast a message with wrong IDs\n"); }
		msg.senderID = envID;
		msg.receiverID = UINT_MAX - 1;
		ret = ipc_broadcast(handle1, &msg);
		if (ret != E_MB_OPER_FAILED) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast a message with wrong IDs\n"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Long message\n") ;
		msg.senderID = envID;
		msg.receiverID = UINT_MAX;
		msg.length = 130;
		ret = ipc_broadcast(handle1, &msg);
		if (ret != E_MB_OPER_FAILED) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to broadcast a long message\n"); }
	}
	eval += is_correct * 10;

	//7. Validation of Create #2
	cprintf_colored(TEXT_cyan, "%~\n7. Validation of Create #2\n") ;
	is_correct = 1;
	{
		char prefix[NAMELEN] = "MB";
		char MBIndex[4];
		char MBName[NAMELEN];
		for (int i = 3; i < MB_MAX_HANDLES; ++i)
		{
			ltostr(i, MBIndex);
			strcconcat(prefix, MBIndex, MBName);
			int handle = sys_ipc_create_mailbox(MBName);
			if (handle < 0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Failed to create MB\n"); }
		}

		cprintf_colored(TEXT_light_cyan, "%~\n	MB Handles List is Full\n") ;
		ltostr(MB_MAX_HANDLES, MBIndex);
		strcconcat(prefix, MBIndex, MBName);
		ret = sys_ipc_create_mailbox(MBName);
		if (ret != E_MB_HANDLES_FULL) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~Error! Attempt to create MB while handles list is FULL\n"); }
	}
	eval += is_correct * 10;


	cprintf_colored(TEXT_light_green, "IPC#2 validations test is completed. Eval = %d\n\n", eval);

}

