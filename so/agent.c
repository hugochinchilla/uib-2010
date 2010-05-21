#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define CLIENTS 10
#define SERVERS 2

typedef struct {
	int value, dirty;
} Cell;

typedef struct {
	int	 code;
	long index;
	long value;
} MessageBody;

typedef struct {
	long type;
	MessageBody data;
} Message;


int queue1, queue2;
int pid_servers[SERVERS];
int clients_finished;
int counter;


void gravedigger()
{
	int buried = 0;

	while (wait3(NULL, WNOHANG, NULL) > 0){
		buried++;
		clients_finished++;
	}
	if (buried > 0) 
		printf("Finished: %d\n", clients_finished);
}

int make_queue(int key)
{
	int queue = msgget(key, IPC_CREAT|0666);

	if(queue < 0 ) {
		perror("msgget");
		exit (0);
	}

	printf ("QUEUE: %d\n", queue);
	return queue;
}

void blowup_queue(int queue)
{
	msgctl(queue, IPC_RMID, 0);
}

void finish(int s)
{
	printf ("exiting %d, messages %d received\n", getpid(), counter);
	exit(0);
}

void servidor()
{
	Message message;
	int res;

	signal (SIGTERM, finish);
	printf ("Server: %d\n", getpid());
	while(1) {
		res = msgrcv(queue1, &message, sizeof(MessageBody), 0, 0);
		if (res < 0) {
			printf ("Reception error %d %s\n", getpid(), strerror(errno));
			continue;
		}
		switch(message.data.code)
		{
			case 0: // Read operation
				printf("Server (%d), value %d received from client %ld\n", getpid(), message.data.value, message.type);
				break;
			case 1: // Write operation
				printf("Server (%d), value %d received from client %ld\n", getpid(), message.data.value, message.type);
				break;
			default:
				printf("Invalid message");
				exit(0);
		}
	}
}


int main()
{
	clients_finished = 0;
	signal(SIGCHLD, gravedigger);

	// Se asigna búfer de línea a stdout
	setlinebuf(stdout);


	exit(0);
}
