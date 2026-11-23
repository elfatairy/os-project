/*
 * ipc1_0.c
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
	struct Message msg1 ;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Temporarily set senderID & receiverID to the current Env
	int32 envIdSender = envID;
	int32 envIdReceiver = envID;

	//Create LONG MESSAGE
	cprintf_colored(TEXT_cyan, "%~\n1. Create LONG MESSAGE\n") ;
	{
		char receiverID[MSG_MAX_SIZE + 10] = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
		ret = ipc_create_data_message((uint8*)receiverID, strlen(receiverID), &msg1);
		if (ret != E_MB_INV_MSG)
			panic("unexpected return! message is invalid");
	}
	//Send MESSAGE with WRONG senderID
	cprintf_colored(TEXT_cyan, "%~\n2. Send MESSAGE with WRONG senderID\n") ;
	{
		char receiverID[MSG_MAX_SIZE + 10];
		ltostr(envIdReceiver, receiverID);
		ret = ipc_create_data_message((uint8*)receiverID, strlen(receiverID), &msg1);
		if (ret != 0)
			panic("Failed to create the message");
		msg1.senderID = envID + 1;
		msg1.receiverID = envIdSender;
		{
			ret = ipc_send(0, &msg1);
			if (ret != E_MB_OPER_FAILED)
				panic("unexpected return! attempt to send message with wrong senderID");
		}
	}
	//Send MESSAGE with WRONG Length
	cprintf_colored(TEXT_cyan, "%~\n3. Send MESSAGE with WRONG Length\n") ;
	{
		char receiverID[MSG_MAX_SIZE + 10];
		ltostr(envIdReceiver, receiverID);
		ret = ipc_create_data_message((uint8*)receiverID, strlen(receiverID), &msg1);
		if (ret != 0)
			panic("Failed to create the message");
		msg1.length = MSG_MAX_SIZE + 1;
		msg1.senderID = envID ;
		msg1.receiverID = envIdSender;
		{
			ret = ipc_send(0, &msg1);
			if (ret != E_MB_OPER_FAILED)
				panic("unexpected return! attempt to send message with invalid length");
		}
	}
	//Receive MESSAGE with WRONG receiverID
	cprintf_colored(TEXT_cyan, "%~\n4. Receive MESSAGE with WRONG receiverID\n") ;
	{
		msg1.receiverID = envID - 1;
		msg1.length = MSG_MAX_SIZE;
		{
			ret = ipc_receive(0, &msg1);
			if (ret != E_MB_OPER_FAILED)
				panic("unexpected return! attempt to receive message with wrong receiverID");
		}
	}
	//Receive MESSAGE with INV Length
	cprintf_colored(TEXT_cyan, "%~\n5. Receive MESSAGE with INV Length\n") ;
	{
		msg1.receiverID = envID;
		msg1.length = MSG_MAX_SIZE + 1;
		{
			ret = ipc_receive(0, &msg1);
			if (ret != E_MB_OPER_FAILED)
				panic("unexpected return! attempt to receive message with invalid length");
		}
	}
	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#1 message validation is completed successfully!!\n\n\n");

}

