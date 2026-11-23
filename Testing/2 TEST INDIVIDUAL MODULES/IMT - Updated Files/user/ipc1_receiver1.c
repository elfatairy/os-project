/*
 * ipc_receiver1.c
 *
 *  Created on: Nov 10, 2026
 *      Author: HP
 */
#include <inc/lib.h>
extern volatile bool printStats;

void _main(void)
{
	printStats = 0;

	int ret ;

	//Env ID
	int envID = myEnv->env_id;
	char myID[NAMELEN];
	ltostr(envID, myID);

	//Receive the message(s) from the sender(s) until a NULL message is received from the master
	struct Message msg1;
	int total = 0;
	int expected = 0;
	do
	{
		msg1.receiverID = envID;
		msg1.length = MSG_MAX_SIZE;
		{
			ret = ipc_receive(0, &msg1);
			if (ret != 0)
				panic("Failed to receive the message");
		}
		if (msg1.type == MSG_DATA)
		{
			total += strtol((char*)msg1.data, NULL, 10);
			expected += msg1.senderID;
		}
		//delay for a while (make receiver slower than senders)
		env_sleep(2000);

	} while (msg1.type != MSG_NULL);

	//Check the result
	if (total != expected)
		panic("error! message content is incorrect");

	//Acknowledge the master
	inctst();
}

