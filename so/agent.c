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
#include <sys/shm.h>
#include <sys/stat.h>

#include "lectores_escritores.h"

#define CLIENTS 10
#define SERVERS 2
#define CLIENT_OPERATIONS 100
#define POOL_SIZE 2

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
int pool;

void reaper()
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
	Cell *memory;
	
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}

	signal (SIGTERM, finish);
	printf ("Server: %d\n", getpid());
	while(1) {
		res = msgrcv(queue1, &message, sizeof(MessageBody), 0, 0);
		if (res < 0) {
			perror("server_rcv");
			continue;
		}
		
		counter++;
		
		switch(message.data.code)
		{
			case 0: // Read operation
				printf("Server (%d), read on [%ld] requested by client %ld\n", getpid(), message.data.index, message.type);
				message_out.data.index = message.data.index;
				message_out.data.value = memory[message.data.index].value;
				message_out.data.code = -1;
				break;
			case 1: // Write operation
				printf("Server (%d), write %ld on [%ld] requested by client %ld\n", getpid(), message.data.value, message.data.index, message.type);
				memory[message.data.index].value = message.data.value;
				memory[message.data.index].dirty = 1;
				message_out.data.index = message.data.index;
				message_out.data.value = message.data.value;
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

/**
 * Makes a request which will be processed by a random server
 * returns 0 if the request was processed without errors, otherwhise
 * it returns 1.
 *
 * When performing a read request, the value parameter will be updated
 * with the value returned by the server so passing it by reference will
 * allow you to recover the read value.
 */
int do_request(int pid, int write, int index, int *value)
{
	int res;
	Message message1, message2;
	
	message1.type = pid;
	message1.data.code = write;
	message1.data.index = index;
	message1.data.value = *value;
	
	res = msgsnd(queue1, &message1, sizeof(MessageBody), 0);
	if (res < 0) {
		perror("client_snd");
		exit(EXIT_FAILURE);
	}
	res = msgrcv(queue2, &message2, sizeof(MessageBody), pid, 0);
	if (res < 0) {
		perror("client_rcv");
		exit(EXIT_FAILURE);
	}
	
	*value = message2.data.value;
	return message2.data.code;
}

void client()
{
	int i, pid, write, index, value, sum, res;
	
	pid = getpid();

	for (i=0; i < CLIENT_OPERATIONS; i++) {
		write = (random() % 5 == 0) ? 1:0;
		write = 1;
		index = random() % POOL_SIZE;
		value = -1;
		
		write ? entrada_escritores() : entrada_lectores();

		res = do_request(pid, 0, index, &value);
		printf("Client (%d), read on [%d] value %d\n", pid, index, value);

		if (write) {
			sum = random() % 10 + 1;
			value +=  sum;
			res = do_request(pid, 1, index, &value);
			printf("Client (%d), read on [%d] value %d after add %d\n", pid, index, value, sum);
		}
		
		write ? salida_escritores() : salida_lectores();
		
		usleep(10000);
	}
}

void initialize_shared_memory()
{
	int i;
	Cell *memory;
	
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<POOL_SIZE; i++){
		memory[i].value = 0;
		memory[i].dirty = 0;
	}

	if (shmdt(memory) == -1) {
		perror("shmdt");
		exit(EXIT_FAILURE);
	}
}

int main()
{
	int i, res;
	finished = 0;
	
	signal(SIGCHLD, reaper);

	// Se asigna búfer de línea a stdout
	setlinebuf(stdout);
	
	queue1 = make_queue(52);
	queue2 = make_queue(53);
	
	pool = shmget(42, POOL_SIZE*sizeof(Cell), IPC_CREAT|S_IRWXU);
	
	initialize_shared_memory();
	inicializar_le();
	
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
	
	// Clients finished, stop servers
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