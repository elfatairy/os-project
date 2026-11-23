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

	int ret;
	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Receive the receiverID from the master program
	struct Message msg1;
	msg1.receiverID = envID;
	msg1.length = MSG_MAX_SIZE;
	{
		ret = ipc_receive(0, &msg1);
		if (ret != 0)
			panic("Failed to receive the message");
	}
	int receiverID = strtol((char*)msg1.data, NULL, 10);

	//Send the message (myID) to the receiver
	struct Message msg2 ;
	ret = ipc_create_data_message((uint8*)myID, strlen(myID), &msg2);
	if (ret != 0)
		panic("Failed to create the message");
	msg2.senderID = envID;
	msg2.receiverID = receiverID;
	{
		ret = ipc_send(0, &msg2);
		if (ret != 0)
			panic("Failed to send the message");
	}

	//Acknowledge the master
	inctst();


}

