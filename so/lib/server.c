#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "commons.h"
#include "lectores_escritores.h"

extern finished, pool, queue1, queue2;

int counter_read, counter_write;


void finish_server(int s)
{
	struct tms ttime;
	times(&ttime);

	printf ("Server exiting %d, %d messages received [read: %d, write: %d] - [sys time: %g, user time: %g]\n",
    getpid(), counter_read + counter_write, counter_read, counter_write, ttime.tms_utime, ttime.tms_stime);
	//finished++;
	exit(EXIT_SUCCESS);
}



void server()
{
	Message message, message_out;
	int res, index, value;
	Cell *memory;
	
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}

	signal (SIGTERM, finish_server);
	signal (SIGINT, SIG_IGN);

	printf ("Server: %d\n", getpid());
	while(1) {
		res = msgrcv(queue1, &message, sizeof(MessageBody), 0, 0);
		if (res < 0) {
			perror("server_rcv");
			continue;
		}

		index = message.data.index;
		value = message.data.value;
		
		switch(message.data.code)
		{
			case 0: // Read operation
				printf("Server (%d), read on [%ld] requested by client %ld\n",
					   getpid(), index, message.type);
                counter_read++;
				message_out.data.index = index;
				message_out.data.value = memory[index].value;
				message_out.data.code = -1;
				break;
			case 1: // Write operation
				printf("Server (%d), write %ld on [%ld] requested by client %ld\n",
					   getpid(), value, index, message.type);
                counter_write++;
				memory[index].value = value;
				memory[index].dirty = 1;
				message_out.data.index = index;
				message_out.data.value = value;
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
