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
int finished;
int counter;


void gravedigger()
{
	int buried = 0;

	while (wait3(NULL, WNOHANG, NULL) > 0){
		buried++;
		finished++;
	}
	if (buried > 0) 
		printf("Finished: %d\n", finished);
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
	finished++;
	exit(0);
}

void server()
{
	Message message, message_out;
	int res;

	signal (SIGTERM, finish);
	printf ("Server: %d\n", getpid());
	while(1) {
		res = msgrcv(queue1, &message, sizeof(MessageBody), 0, 0);
		if (res < 0) {
			printf ("Reception error %d %s\n", getpid(), strerror(errno));
			continue;
		}
		counter++;
		switch(message.data.code)
		{
			case 0: // Read operation
				printf("Server (%d), read on %d requested by client %ld\n", getpid(), message.data.index, message.type);
				message_out.data.index = message.data.index;
				message_out.data.value = 5;
				message_out.data.code = -1;
				break;
			case 1: // Write operation
				printf("Server (%d), write %d on %d requested by client %ld\n", getpid(), message.data.value, message.data.index, message.type);
				message_out.data.index = message.data.index;
				message_out.data.value = 6;
				message_out.data.code = 0;
				break;
			default:
				printf("Invalid message\n");
		}
		message_out.type = message.type;
		res = msgsnd(queue2, &message_out, sizeof(MessageBody), 0);
		if (res < 0) {
			perror("server_response");
		}
	}
}

void client()
{
	int i, res;
	Message message1, message2;

	for (i=0; i < 100; i++) {
		message1.type = getpid();
		message1.data.code = 1;
		message1.data.index = 10;
		message1.data.value = 42;
		
		res = msgsnd(queue1, &message1, sizeof(MessageBody), 0);
		if (res < 0) {
			printf("Error in delivery, %d, %d, %s\n", getpid(), queue1, strerror(errno));
		}
		res = msgrcv(queue2, &message2, sizeof(MessageBody), getpid(), 0);
		if (res < 0) {
			printf ("Error receiving %d, %d, %s\n", getpid(), queue2, strerror(errno));
		}
		printf("Client (%d): value = %d\n", getpid(), message2.data.value);
		usleep(10000);
	}
}


int main()
{
	int i, res;
	finished = 0;
	
	signal(SIGCHLD, gravedigger);

	// Se asigna búfer de línea a stdout
	setlinebuf(stdout);
	
	queue1 = make_queue(52);
	queue2 = make_queue(53);
	
	printf("Starting servers\n");
	for (i=0; i < SERVERS; i++) {
		pid_servers[i] = fork();
		if (pid_servers[i] < 0){
			perror("fork_server");
		}
		if (pid_servers[i] == 0){
			server();
			exit(0);
		}
	}
	
	printf("Starting clients\n");
	for(i=0; i < CLIENTS; i++) {
		//printf("client iter: %d\n", i);
		res = fork();
		if (res < 0) {
			perror("fork_client");
		}
		if (res == 0){ // CHILD
			client();
			exit(0);
		}
		usleep(10000);
	}
	
	// Wait all clients to finish
	while (finished < CLIENTS) {
		pause();
	}
	
	// Client finished, stop servers
	for (i=0; i < SERVERS; i++) {
		printf("Sending SIGTERM to server %d\n", pid_servers[i]);
		kill(pid_servers[i], SIGTERM);
	}
	
	// Wait for servers to finish
	while (finished < (CLIENTS + SERVERS))  {
		pause();
	}
	
	blowup_queue(queue1);
	blowup_queue(queue2);
	return 0;
}
