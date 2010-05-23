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

extern counter, finished, pool;

void dump_memory()
{
	int i;
	Cell *memory;
	
    printf("BEGIN WRITER\n");
    entrada_escritores();
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<POOL_SIZE; i++){
		printf("%d, ", memory[i].value);
		memory[i].dirty = 0;
	}

	if (shmdt(memory) == -1) {
		perror("shmdt");
		exit(EXIT_FAILURE);
	}
    salida_escritores();
    printf("\nEND WRITER\n");
}

void finish_writer()
{
    dump_memory();
    exit(EXIT_SUCCESS);
}

void writer()
{
    signal(SIGTERM, finish_writer);
    
    while (1){
        dump_memory();
        usleep(50000);
    }
}