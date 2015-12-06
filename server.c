/*
*	Oğuz Demir 21201712 CS342-1
*	Project2- server file
*	Takes request from clients and create a child process for each request.
*	In multi threaded manner, it opens given files and finds the matched words.
*	Result is given back to the client. In communication, POSIX message queues are used.
*
*	To implement pthread functions, definitions are taken from lecture slides. 
*	Global variables are used for communication between threads.
*	For usage of message queues, manual of posix message queues (given in piazza),ubuntuforums and gnu.org is used.
*	I also researched for examples of usage of message queues. 
*	
*/
#define _GNU_SOURCE
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>
#include <signal.h> 
#include <unistd.h> 

//In piazza, it is guaranteed that program won't be tested for huge files. 
//So I put 1000 character limit for the size of reply. 
//I couldnt use a constant variable because static arrays cannot be declared with variables in C.
//1000 number in used 3 times, in global variable declaration, struct definiton and temp variable in thread.
//So all of these should be changed if maximum size is wanted to be increased.
char results[10][1000];	
char keyword[128];

//Number of files is hold as global, because threads should know which part of the results array is used.
//For example if there is 5 files, other 5 indexes will be null and unintended test of these would cause seg fault. 
int numberOfFiles;

//Struct that holds the request's data
typedef struct
{
	char queueName[128];
	char keyword[128];
	int fileNumber;
	char files[10][128];
} RequestMessage ;	

//Struct that holds the reply's data.
//It can be managed with one string, but I preferred to use struct to modify easily if needed.
typedef struct
{
	char report[1000];
} ReplyMessage; 

//Exist variable controls the loop.
//When Ctrl+C signal is sent to server, it should delete the queue.
//So termination is controlled in that way.
int exits = 0;
void sigHandler(int sig)
{
	printf("SIGNAL RECIEVED\n");
	exits = 1;
}

//Thread function
void *threadFuncx (void *arg)
{
	//Filenames are passed to thread in 2 ways. The arg includes the filename that is going to be processed by that thread.
	//Also, results[i] includes the i'th thread's filename.
	//That way, a thread can know which position of the array is belong to it. 
	//So that, for example if the name came in the results[5] pointer, thread can record the results also in results[5].
	//Return value of the thread function can also be used to pass the results to main, but I preferred to use global variables. 

	//Filename
	char str[128];
	strcpy(str,(char*)arg);
	
	//Finding which file is processing in that thread
	//So that report can be put the corresponding position
	int a;
	for(a = 0; a < numberOfFiles ; a++)
	{
		if(strcmp(results[a],str) == 0)
		{
			break;
		}
	}

	//Trying to open file
	FILE *fp;
	fp = fopen(str,"r");
	if(fp== NULL)	//If pointer is null, there is no such file
	{
		sprintf (results[a],"<%s> [!]: File not found!",str);
		pthread_exit(0);
	}


	int linecount = 0;
	
	//Temp string that holds the result of corresponding file.
	//This size should be also changed if maximum size of the reply is changed.
	char temp[1000] = "";

	//Count of matching word.
	int count = 0;

	//Buffer for line
	char line[128];

	//Reading the file one by one
	while (fgets(line, sizeof(line),fp) != NULL)	//Loop until end of file
	{	
		linecount++;
		//To catch the exact words,
		//Begining and end of the words are determined.
		//If length of catched word is same with keyword length, characters are 1-1 compared and
		//if all are the same, word is found.

		int i,firstIndex,secondIndex;			
		for(i = 0; i < strlen(line); i++)
		{
			while(line[i]=='\n' || line[i]=='\0' ||  line[i]=='\t' || line[i] ==' ')
				i++;
			firstIndex = i;
			i++;
 
			while(!(line[i]=='\n' || line[i]=='\0' ||  line[i]=='\t' || line[i] == ' '))
				i++;
			
			secondIndex = i-1;
	
			if( firstIndex >= strlen(line)  || secondIndex >= strlen(line) )
				break;
			
			//If size doesn't match, moving to the next word
			if(strlen(keyword) != secondIndex-firstIndex + 1)
				continue;
			else
			{
				int j;
				int flag = 1;
				for(j = 0 ; j < strlen(keyword) ; j++)
				{
					if(keyword[j] != line[firstIndex + j])
					{
						flag = 0;
						break;	
					}
				}
				//If a match found, line number is added to temp string and count is increased.
				if(flag == 1)
				{
					count++;
					char pd[10];
					sprintf(pd, " %d", linecount);
					strcat(temp,pd);
				}
					
			} 
		}

	}
	//Formating the result.
	sprintf (results[a],"<%s> [%d]: %s",str,count,temp);
	pthread_exit(0);

}

int main(int argc, char *argv[])
{
	//Defining signal handler for breaking the loop
	signal(SIGINT, sigHandler);

	//Defining 2 queues for request and reply. 
	mqd_t server, sender;


	struct mq_attr attributes;
	//Attributes for server queue.
	attributes.mq_flags =  O_NONBLOCK;
	attributes.mq_maxmsg = 5;
	attributes.mq_msgsize = sizeof(RequestMessage);
	attributes.mq_curmsgs = 0;


	//Permisions for queues, taken from gnu.org
	//It includes read&write permissions for owner, group owner and other users in order.
	mode_t perms  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;


	//Server queue is opened with O_CREAT property to open non-existing one.
	//The receive will be done in infinite loop, so it should recieve non-blockingly.
	//Server will make only read operation, so it opened with read only flag.
	server = mq_open(argv[1], O_CREAT | O_RDONLY | O_NONBLOCK, perms , &attributes);
	
	if(server == -1)
	{
		printf("Server's queue cannot be created. Error: %s (%d)\n", strerror(errno), errno);
		return 1;
	}

	while(exits!= 1)
	{	
		RequestMessage msg;

		//If a message is recieved...
		if( mq_receive(server, (char *)&msg , sizeof(RequestMessage), NULL) > 0)
		{
			//Create a child process for each request.
			pid_t  a = 0;
			a = fork();

			//Child process work
			if (a == 0)
			{
				//keyword and #ofFiles are copied to global variable for allowing threads to use it.
				strcpy(keyword,msg.keyword);
				numberOfFiles = msg.fileNumber;

				//At the end of process 2D results array will hold the each thread's report.
				//So, for each filename, one index of that array is reserved by copying the filename to an index.
				int k;
				for(k=0; k< numberOfFiles ; k++)
				{
					strcpy(results[k],msg.files[k]);
				}

				//Threads are creating with default attributes.
				pthread_t tid;
				pthread_t tids[10];
				pthread_attr_t attr;  
				pthread_attr_init (&attr); 

				for(k=0; k< numberOfFiles ; k++)
				{
					pthread_create (&tid, &attr, threadFuncx, msg.files[k]);
					tids[k] = tid;
				}

				//Waiting for threads
				for(k=0; k< numberOfFiles ; k++)
				{
					tid = tids[k];
					pthread_join (tid, NULL); 
				}
				
				//Initializing the reply struct.	
				ReplyMessage sent;
				strcpy(sent.report,"");

				//Merging the thread reports.
				for(k=0; k< numberOfFiles ; k++)
				{
					strcat(sent.report,results[k]);
					strcat(sent.report,"\n");
				}
				
				
				//Unique client queuename is taken from message
				char queueName[128];
				strcpy(queueName,msg.queueName);

				//Send the struct to client and close the queue.
				sender = mq_open(queueName,  O_WRONLY | O_NONBLOCK);

				if (mq_send(sender, (char*)&sent, sizeof(ReplyMessage), 0) == -1)
				{
					printf("Error sending reply: %s (%d)\n", strerror(errno), errno);
				}
				mq_close(sender);

				//Terminate the child process
				exit(0);
			}
		}
	}	//Continue until interrupt

	//When interrupt signal comes, close and delete the queue
	mq_close(server);
  	mq_unlink(argv[1]);

	return 0;


}
