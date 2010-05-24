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

extern finished, pool, queue1, queue2;

int counter_read, counter_write;

void finish(int s)
{
	printf ("Server exiting %d, %d messages received [read: %d, write: %d] \n",
    getpid(), counter_read + counter_write, counter_read, counter_write);
	finished++;
	exit(EXIT_SUCCESS);
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
		
		switch(message.data.code)
		{
			case 0: // Read operation
				printf("Server (%d), read on [%ld] requested by client %ld\n",
					   getpid(), message.data.index, message.type);
                counter_read++;
				message_out.data.index = message.data.index;
				message_out.data.value = memory[message.data.index].value;
				message_out.data.code = -1;
				break;
			case 1: // Write operation
				printf("Server (%d), write %ld on [%ld] requested by client %ld\n",
					   getpid(), message.data.value, message.data.index, message.type);
                counter_write++;
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