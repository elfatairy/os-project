/*
 * ipc2_receiver1.c
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

	//Connect to ALL four mailboxes from the parent
	int handleMBforAck = ipc_connect(myEnv->env_parent_id, "MBforAck", MB_RECEIVER);
	if (handleMBforAck < 0) panic ("Failed to connect to MBforAck") ;
	int handleMBforAll = ipc_connect(myEnv->env_parent_id, "MBforAll", MB_RECEIVER);
	if (handleMBforAll < 0) panic ("Failed to connect to MBforAll") ;
	int handleMBforEven = ipc_connect(myEnv->env_parent_id, "MBforEven", MB_RECEIVER);
	if (handleMBforEven < 0) panic ("Failed to connect to MBforEven") ;
	int handleMBforOdd = ipc_connect(myEnv->env_parent_id, "MBforOdd", MB_RECEIVER);
	if (handleMBforOdd < 0) panic ("Failed to connect to MBforOdd") ;

	//Check the number of handles inside the env
	int numOfHandles = LIST_SIZE(&(myEnv->mailboxHandles.mailbox_handles_list)) ;
	expected = 4;
	if (numOfHandles != expected)
		panic("Invalid number of handles! Expected = %d, Actual = %d", expected, numOfHandles);

	int numOfSenders = 0;
	int numOfReceivers = 0;
	for (int i = 0; i < MB_MAX_HANDLES; ++i)
	{
		if (myEnv->__mbhs__[i].connectionType == MB_SENDER) numOfSenders++;
		if (myEnv->__mbhs__[i].connectionType == MB_RECEIVER) numOfReceivers++;
	}
	if (numOfSenders != 0 || numOfReceivers != 4)
		panic("Unexpected number of senders/receivers handles");

	//Acknowledge the parent
	inctst();

	//Wait for a ack signal from the master
	char ackCmd[NAMELEN] = "__IsProcStopped__";
	int response = 0;
	while (!response)
	{
		sys_utilities(ackCmd, (uint32)&response);
		env_sleep(500);
	}

//	//Wait for a NULL broadcasted message from the parent
//	struct Message msg;
//	msg.receiverID = UINT_MAX ;
//	{
//		ret = ipc_receive(0, &msg);
//		if (ret < 0) panic("Failed to receive a NULL message from the local mailbox");
//		if (msg.type != MSG_NULL) panic("Unexpected message! expected NULL msg from the parent");
//	}

	//Disconnect from the Even/Odd mailbox according to envID
	if (envID % 2 == 0)
	{
		ipc_disconnect(handleMBforOdd);
	}
	else
	{
		ipc_disconnect(handleMBforEven);
	}

	//Check the number of handles inside the env
	numOfHandles = LIST_SIZE(&(myEnv->mailboxHandles.mailbox_handles_list)) ;
	expected = 3;
	if (numOfHandles != expected)
		panic("Invalid number of handles! Expected = %d, Actual = %d", expected, numOfHandles);

	numOfSenders = 0;
	numOfReceivers = 0;
	for (int i = 0; i < MB_MAX_HANDLES; ++i)
	{
		if (myEnv->__mbhs__[i].connectionType == MB_SENDER) numOfSenders++;
		if (myEnv->__mbhs__[i].connectionType == MB_RECEIVER) numOfReceivers++;
	}
	if (numOfSenders != 0 || numOfReceivers != 3)
		panic("Unexpected number of senders/receivers handles");

	//Acknowledge the parent
	inctst();
}

