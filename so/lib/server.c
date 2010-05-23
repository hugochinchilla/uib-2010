#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "commons.h"
#include "lectores_escritores.h"

extern counter, finished, pool, queue1, queue2;

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
				printf("Server (%d), read on [%ld] requested by client %ld\n",
					   getpid(), message.data.index, message.type);
				message_out.data.index = message.data.index;
				message_out.data.value = memory[message.data.index].value;
				message_out.data.code = -1;
				break;
			case 1: // Write operation
				printf("Server (%d), write %ld on [%ld] requested by client %ld\n",
					   getpid(), message.data.value, message.data.index, message.type);
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