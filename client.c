/*
*	Oğuz Demir 21201712 CS342-1
*	Project2- client file
*	For request, Client is passing the command line arguments to the server in a struct via message queue.
*	It prints the results which is sent by server through another message queue.
*	So all client has to do is arranging an object contains the request information, passing it server, recieving the result and print it.
*	More explanations about queues are located in header of server file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h> 

typedef struct
{
	char queueName[128];
	char keyword[128];
	int fileNumber;
	char files[10][128];
} RequestMessage;	

//Maximum size of reply declared as 1000, as mentioned in server file.
typedef struct
{
	char report[1000];
} ReplyMessage;	

pid_t getpid(void); 

int main(int argc, char *argv[])
{
	//Creating client specific queue name
	char str[20];
	sprintf(str, "/queue%ld", (long)getpid());
	
	RequestMessage msg;

	strcpy(msg.queueName,str);
	strcpy(msg.keyword,argv[2]);
	msg.fileNumber = atoi(argv[3]);
	if(msg.fileNumber + 4 != argc || msg.fileNumber > 10 || msg.fileNumber < 1 )
	{
		printf("Argument failure, termination\n");
		return 0;
	}
	int i;
	for(i = 0 ; i < msg.fileNumber ; i++)
	{
		strcpy(msg.files[i],argv[4+i]);
	}

	mqd_t unique, server;

	//Attributes for client queue.
	struct mq_attr attributes;
	attributes.mq_flags = O_NONBLOCK;
	attributes.mq_maxmsg = 1;
	attributes.mq_msgsize = sizeof(ReplyMessage);
	attributes.mq_curmsgs = 0;
	
	//Permisions for queues, taken from gnu.org
	//It includes read&write permissions for owner, group owner and other users in order.
	mode_t perms  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	//Opening client specific message queue
	unique = mq_open(str, O_CREAT | O_RDONLY | O_NONBLOCK, perms, &attributes);
	if(unique == -1)
	{
		printf("Unique queue creation error: %s (%d)\n", strerror(errno), errno);	
		return 1;
	}

	//Opening the queue that server created
	server = mq_open(argv[1], O_WRONLY | O_NONBLOCK);
	if(server == -1)
	{
		printf("Server queue opeining error: %s (%d)\n", strerror(errno), errno);	
		return 1;
	}

	//Sending the request
	if (mq_send(server, (char*) &msg, sizeof(RequestMessage), 0) == -1)
	{
		printf("Request send error: %s (%d)\n", strerror(errno), errno);
	}

	//Closing the queue
	mq_close(server);

	//Try to receive the server's reply until it comes.
	//Loop will break when a message is taken
	while (1)
	{
		ReplyMessage recieved; 

		if(mq_receive(unique, (char *)&recieved , sizeof(ReplyMessage), NULL)>0)
		{
			printf("keyword: %s\n",argv[2]);
			printf("%s\n", recieved.report);
			break;
		}
	}
	mq_unlink(str);
	mq_close(unique);
	return 0;

}
