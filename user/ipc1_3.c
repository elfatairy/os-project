/*
 * ipc1_3.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>
extern volatile bool printStats;

#define NUM_OF_SENDERS 20
void _main(void)
{
	printStats = 0;
	rsttst();
	int ret;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Load Many Senders & One Receiver
	cprintf_colored(TEXT_cyan, "%~\n1. Load Many Senders & One Receiver\n") ;
	int32 envIdSenders[NUM_OF_SENDERS];
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		envIdSenders[i] = sys_create_env("ipc1_sender1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
		if (envIdSenders[i] == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs...");
	}
	int32 envIdReceiver = sys_create_env("ipc1_receiver1", (myEnv->page_WS_max_size),(myEnv->page_2nd_WS_size) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	if (envIdReceiver == E_ENV_CREATION_ERROR)
		panic("NO AVAILABLE ENVs...");

	//Send receiverID to all senders before starting them [non-blocking]
	cprintf_colored(TEXT_cyan, "%~\n2. Send receiverID to all senders before starting them [non-blocking]\n") ;
	char receiverID[NAMELEN];
	ltostr(envIdReceiver, receiverID);
	struct Message msg1 ;
	ret = ipc_create_data_message((uint8*)receiverID, strlen(receiverID), &msg1);
	if (ret != 0)
		panic("Failed to create the message");

	msg1.senderID = envID;
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		msg1.receiverID = envIdSenders[i];
		ret = ipc_send(0, &msg1);
		if (ret != 0)
			panic("Failed to send the message");
	}
	//Run ALL Senders
	cprintf_colored(TEXT_cyan, "%~\n3. Run ALL Senders\n") ;
	for (int i = 0; i < NUM_OF_SENDERS; ++i)
	{
		sys_run_env(envIdSenders[i]);
	}

	//Wait until ALL senders send message(s) to receiver (non-blocking)
	cprintf_colored(TEXT_cyan, "%~\n4. Wait until ALL senders send message(s) to receiver (non-blocking)\n") ;
	while (gettst() != NUM_OF_SENDERS);

	//Run Receiver
	cprintf_colored(TEXT_cyan, "%~\n5. Run Receiver\n") ;
	sys_run_env(envIdReceiver);

	//Send a NULL message to the receiver
	cprintf_colored(TEXT_cyan, "%~\n6. Send a NULL message to the receiver\n") ;
	struct Message msg2 ;
	msg2.type = MSG_NULL ;
	msg2.senderID = envID;
	msg2.receiverID = envIdReceiver;
	{
		ret = ipc_send(0, &msg2);
		if (ret != 0)
			panic("Failed to send the message");
	}

	//Wait until acknowledgment from the receiver
	cprintf_colored(TEXT_cyan, "%~\n7. Wait until acknowledgment from the receiver\n") ;
	while (gettst() != NUM_OF_SENDERS + 1);

	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#1 M-1 non-blocking model is completed successfully!!\n\n\n");

}

