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
    FILE *bdata;
	Cell *memory;
    Cell value;
	
    printf("BEGIN WRITER\n");
    entrada_escritores();
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
    
    // open the file
    //bdata = fopen(binary_filename, "w");
    bdata = fopen("data.bin", "w");
	
	for (i=0; i<POOL_SIZE; i++){
        value = memory[i];
        printf("%d, ", value.value);
        
        if (value.dirty){
            fseek(bdata, sizeof(Cell)*i, SEEK_SET);
            fwrite(&value, sizeof(Cell), 1, bdata);
            memory[i].dirty = 0;
        }
	}
    
    // close the file
    fclose(bdata);

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
        usleep(5000000);
    }
}