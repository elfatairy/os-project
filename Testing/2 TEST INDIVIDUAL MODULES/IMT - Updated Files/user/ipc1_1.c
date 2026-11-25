/*
 * ipc1_1.c
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

	//Load Sender & Receiver
	cprintf_colored(TEXT_cyan, "%~\n1. Load Sender & Receiver\n") ;

	int32 envIdSender = sys_create_env("ipc1_sender1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	int32 envIdReceiver = sys_create_env("ipc1_receiver1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	if (envIdSender == E_ENV_CREATION_ERROR || envIdReceiver == E_ENV_CREATION_ERROR)
		panic("NO AVAILABLE ENVs...");

	//Send receiverID to the sender before starting it [non-blocking]
	cprintf_colored(TEXT_cyan, "%~\n2. Send receiverID to the sender before starting it [non-blocking]\n") ;
	char receiverID[NAMELEN];
	ltostr(envIdReceiver, receiverID);
	struct Message msg1 ;
	ret = ipc_create_data_message((uint8*)receiverID, strlen(receiverID), &msg1);
	if (ret != 0)
		panic("Failed to create the message");
	msg1.senderID = envID;
	msg1.receiverID = envIdSender;
	{
		ret = ipc_send(0, &msg1);
		if (ret != 0)
			panic("Failed to send the message");
	}

	//Run Sender

	//Wait until the sender send message(s) to receiver (non-blocking)
	cprintf_colored(TEXT_cyan, "%~\n3. Wait until the sender send message(s) to receiver (non-blocking)\n") ;
	sys_run_env(envIdSender);
	while (gettst() != 1);

	//Send a NULL message to the receiver
	cprintf_colored(TEXT_cyan, "%~\n4. Send a NULL message to the receiver\n") ;
	struct Message msg2 ;
	msg2.type = MSG_NULL ;
	msg2.senderID = envID;
	msg2.receiverID = envIdReceiver;
	{
		ret = ipc_send(0, &msg2);
		if (ret != 0)
			panic("Failed to send the message");
	}

	//Run Receiver
	//Wait until acknowledgment from the receiver
	cprintf_colored(TEXT_cyan, "%~\n5. Wait until acknowledgment from the receiver\n") ;
	sys_run_env(envIdReceiver);
	while (gettst() != 2);

	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#1 1-1 non-blocking model is completed successfully!!\n\n\n");

}

