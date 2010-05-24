#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "lectores_escritores.h"
#include "commons.h"

extern queue1, queue2, finished;


/**
 * Makes a request which will be processed by a random server
 * returns 0 if the request was processed without errors, otherwise
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

void finish_client()
{
    //finished++;
	exit(EXIT_SUCCESS);
}

void client()
{
	int i, pid, write, index, value, sum, res;
	
	signal(SIGTERM, finish_client);
	signal (SIGINT, SIG_IGN);
	
	pid = getpid();

    
	for (i=0; i < CLIENT_OPERATIONS; i++) {
        // Seed the random number generator with a different value for each client and operation
        srand(i * pid);
        
		write = (random() % 5 == 0) ? 1:0;
		index = random() % POOL_SIZE;
		value = -1;
		
		write ? entrada_escritores() : entrada_lectores();

		res = do_request(pid, 0, index, &value);
		printf("Client (%d), read on [%d] value %d\n", pid, index, value);

		if (write) {
			sum = random() % 10 + 1;
			value +=  sum;
			res = do_request(pid, 1, index, &value);
			printf("Client (%d), read on [%d] value %d after add %d\n",
                   pid, index, value, sum);
		}
		
		write ? salida_escritores() : salida_lectores();
		
		usleep(10000);
	}
}
