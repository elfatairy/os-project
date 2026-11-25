/*
 * ipc2_sender2.c
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
	struct Message msg;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Connect to ALL mailboxes from the parent
	int handleMBforAck = ipc_connect(myEnv->env_parent_id, "MBforAck", MB_RECEIVER);
	if (handleMBforAck < 0) panic ("Failed to connect to MBforAck. ret = %d",handleMBforAck) ;
	int handleMB1 = ipc_connect(myEnv->env_parent_id, "MB1", MB_SENDER);
	if (handleMB1 < 0) panic ("Failed to connect to MB1. ret = %d",handleMB1) ;
	int handleMB2 = ipc_connect(myEnv->env_parent_id, "MB2", MB_SENDER);
	if (handleMB2 < 0) panic ("Failed to connect to MB2. ret = %d",handleMB2) ;

	//Acknowledge the master
	inctst();

	//Wait for a ack signal from the master
	char ackCmd[NAMELEN] = "__IsProcStopped__";
	int response = 0;
	while (!response)
	{
		sys_utilities(ackCmd, (uint32)&response);
		env_sleep(500);
	}

//	//Wait for a NULL broadcasted message from the master
//	struct Message msg;
//	msg.receiverID = UINT_MAX ;
//	{
//		ret = ipc_receive(0, &msg);
//		if (ret < 0) panic("Failed to receive a NULL message from the local mailbox");
//		if (msg.type != MSG_NULL) panic("Unexpected message! expected NULL msg from the parent");
//	}

	//Broadcast the envID to all registered receivers
	msg.type = MSG_DATA;
	msg.senderID = envID;
	msg.receiverID = UINT_MAX;
	msg.length = strlen(myID);
	strcpy((char*)msg.data, myID);
	{
		ipc_broadcast(handleMB1, &msg);
		ipc_broadcast(handleMB2, &msg);
	}

	//Acknowledge the master again
	inctst();
}

