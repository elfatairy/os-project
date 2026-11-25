/*
 * ipc2_0_slave.c
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

	//Create Mailboxes
	cprintf_colored(TEXT_cyan, "%~\n1. Create Mailboxes\n") ;
	int handle1 = sys_ipc_create_mailbox("MB1");
	if (handle1 < 0) {panic("%~Failed to create MB\n"); }
	int handle2 = sys_ipc_create_mailbox("MB2");
	if (handle2 < 0) {panic("%~Failed to create MB\n"); }
	int handle3 = sys_ipc_create_mailbox("MB3");
	if (handle3 < 0) {panic("%~Failed to create MB\n"); }

	char token[20] ;
	char separator[2] = "@";
	char getMBCmd[NAMELEN] = "__GetMailbox@";
	char cmd1[NAMELEN]  ;

	//Connect to the Master Mailboxes
	int senderHandle, receiverHandle ;
	cprintf_colored(TEXT_cyan, "%~\n2. Connect to the Master Mailboxes\n") ;
	{
		senderHandle = ipc_connect(myEnv->env_parent_id, "MainMB1", MB_SENDER);
		receiverHandle = ipc_connect(myEnv->env_parent_id, "MainMB2", MB_RECEIVER);
	}
	//Test Successful Cases of Get Mailbox
	cprintf_colored(TEXT_cyan, "%~\n3. Test Successful Cases of Get Mailbox\n") ;
	{
		cprintf_colored(TEXT_light_cyan, "%~\n	a. Owner and get as Owner\n") ;
		ltostr(handle1, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_OWNER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != 0) { panic("ERROR: failed to get mailbox as owner!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Owner and get as Sender\n") ;
		ltostr(handle2, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_SENDER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != 0) { panic("ERROR: failed to get mailbox as sender!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	c. Owner and get as Receiver\n") ;
		ltostr(handle3, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_RECEIVER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != 0) { panic("ERROR: failed to get mailbox as receiver!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	d. Sender and get as Sender\n") ;
		ltostr(senderHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_SENDER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != 0) { panic("ERROR: failed to get mailbox as sender!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	e. Receiver and get as Receiver\n") ;
		ltostr(receiverHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_RECEIVER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != 0) { panic("ERROR: failed to get mailbox as receiver!"); }

	}

	//Test Failure Cases of Get Mailbox
	cprintf_colored(TEXT_cyan, "%~\n4. Test Failure Cases of Get Mailbox\n") ;
	{
		cprintf_colored(TEXT_light_cyan, "%~\n	a. mailbox handle not exist\n") ;
		ltostr(handle1+1, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_OWNER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != E_MB_NOT_EXIST) { panic("ERROR: attempt to access a non-exist mailbox!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	b. Sender and get as Owner (Inv Ownership)\n") ;
		ltostr(senderHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_OWNER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != E_MB_INV_OWNER) { panic("ERROR: attempt to access a mailbox with invalid ownership!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	c. Receiver and get as Owner (Inv Ownership)\n") ;
		ltostr(receiverHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_OWNER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != E_MB_INV_OWNER) { panic("ERROR: attempt to access a mailbox with invalid ownership!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	d. Sender and get as Receiver\n") ;
		ltostr(senderHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_RECEIVER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != E_MB_INV_REQUEST) { panic("ERROR: attempt to access a mailbox with wrong connection!"); }

		cprintf_colored(TEXT_light_cyan, "%~\n	e. Receiver and get as Sender \n") ;
		ltostr(receiverHandle, token);strcconcat(getMBCmd, token, cmd1);strcconcat(cmd1, separator, cmd1);
		ltostr(MB_SENDER, token);strcconcat(cmd1, token, cmd1);
		sys_utilities(cmd1, (uint32)&ret);
		if (ret != E_MB_INV_REQUEST) { panic("ERROR: attempt to access a mailbox with wrong connection!"); }
	}

	cprintf_colored(TEXT_light_green, "Congratulations!! IPC#2 test get mailbox completed successfully!!\n\n\n");

}

