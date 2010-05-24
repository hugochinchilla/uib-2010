#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "lib/commons.h"
#include "lib/lectores_escritores.h"

int queue1, queue2;
int pid_servers[SERVERS];
int pid_clients[CLIENTS];
int pid_writer;
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

void initialize_shared_memory()
{
	int i;
    FILE *bdata;
	Cell *memory;
    Cell value;
    
    value.value = 0;
    
    bdata = fopen(binary_filename, "r");
    
    
    // File does not exist, init one
    if (bdata == 0){
        value.value = 0;
        value.dirty = 0;
        
        bdata = fopen(binary_filename, "w");
        
        for (i=0;i<POOL_SIZE;i++){
            fwrite(&value, sizeof(Cell), 1, bdata);
        }
        
        // Reset file to read mode
        fclose(bdata);
        bdata = fopen(binary_filename, "r");
    }
	
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
    
	for (i=0; i<POOL_SIZE; i++){
        fread(&value,sizeof(Cell),1,bdata);
		memory[i].value = value.value;
		memory[i].dirty = value.dirty;
	}
    
    fclose(bdata);
    
    /*
    for (i=0; i<POOL_SIZE; i++){
		printf("%d,", memory[i].value);
    }
    printf("\n");
    exit(0);
    */

	if (shmdt(memory) == -1) {
		perror("shmdt");
		exit(EXIT_FAILURE);
	}
}

void finish_agent()
{
	int i;
    
	
    //printf("\n============== WAITING CLIENTS ===================\n");
	// Wait all clients to finish
	while (finished < CLIENTS) {
        //printf("PAUSE EXEC %d\n", finished);
		pause();
	}
    //printf("\n============== FINISH AGENT INVOKE ===================\n");
    //printf("\n============== CLIENTS FINISHED ===================\n");
	
	// Clients finished, stop servers
	for (i=0; i < SERVERS; i++) {
		printf("Sending SIGTERM to server %d\n", pid_servers[i]);
		kill(pid_servers[i], SIGTERM);
	}
	
	// Stop writer
	kill(pid_writer, SIGTERM);
	
    
    //printf("\n============== WAITING ===================\n");
	// Wait for servers and writer to finish
	while (finished < (CLIENTS + SERVERS + 1))  {
        //printf("WAITING: %d\n", finished);
		pause();
	}
    //printf("\n============== ALL TERMINATED ===================\n");

	// Remove message queues
	blowup_queue(queue1);
	blowup_queue(queue2);
	
	// Remove shared memory
    if (shmctl(pool, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

void interrupt_agent()
{
	int i;
	
	// Kill clients
	for(i=0; i < CLIENTS; i++) {
		kill(pid_clients[i], SIGINT);
	}
    
	finish_agent();
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    /*
    if (argc != 2) {
        printf("usage: agent binary.bin\n");
        //printf("usage: agent binary.bin logfile.txt\n");
        exit(EXIT_SUCCESS);
    }
    */
    
	int i;
	finished = 0;
    
    //binary_filename = argv[1];
	
	signal(SIGCHLD, reaper);
	signal(SIGINT, interrupt_agent);

	setlinebuf(stdout);
	
	// Create message queues
	queue1 = make_queue(52);
	queue2 = make_queue(53);
	
	// Create the shared memory space
	pool = shmget(53, POOL_SIZE*sizeof(Cell), IPC_CREAT|S_IRWXU);
	
	initialize_shared_memory();
	inicializar_le();
	
	// Start servers
	for (i=0; i < SERVERS; i++) {
		pid_servers[i] = fork();
		if (pid_servers[i] < 0){
			perror("fork_server");
		}
		if (pid_servers[i] == 0){
			server();
			exit(EXIT_SUCCESS);
		}
	}
	
	// Start writer
	pid_writer = fork();
	if (pid_writer < 0){
		perror("fork_writer");
		exit(EXIT_FAILURE);
	}
	if (pid_writer == 0){
		writer();
		exit(EXIT_SUCCESS);
	}
	
	// Start clients
	for(i=0; i < CLIENTS; i++) {
		pid_clients[i] = fork();
		if (pid_clients[i] < 0) {
			perror("fork_client");
		}
		if (pid_clients[i] == 0){ // CHILD
			client();
			exit(EXIT_SUCCESS);
		}
		usleep(10000);
	}
	
	finish_agent();
	return 0;
}