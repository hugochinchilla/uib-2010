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
	int i, value, runs, dirty;
    FILE *bdata;
	Cell *memory;

    printf("BEGIN WRITER\n");
    entrada_lectores();
	memory = shmat(pool, NULL, 0);
	if (memory == (void *)-1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
    
    // open the file and read writer runs from binary file
    bdata = fopen(binary_filename, "r");
	fread(&runs, sizeof(int), 1, bdata);

	// Reset file to write mode
	fclose(bdata);
    bdata = fopen(binary_filename, "w");

	runs++;

	// update writer runs
    fseek(bdata, 0, SEEK_SET);
	fwrite(&runs, sizeof(int), 1, bdata);
	
	for (i=0; i<POOL_SIZE; i++){
		value = memory[i].value;
		dirty = memory[i].dirty;
        printf("(%d,%d), ", value, dirty);
        
        if (dirty){
			fseek(bdata, sizeof(int)*(i+1), SEEK_SET);
            fwrite(&value, sizeof(int), 1, bdata);
            memory[i].dirty = 0;
        }		
	}
    
    // close the file
    fclose(bdata);

	if (shmdt(memory) == -1) {
		perror("shmdt");
		exit(EXIT_FAILURE);
	}
    salida_lectores();
    printf("\nEND WRITER EXECUTION No %d\n", runs);
}

void finish_writer()
{
	//finished++;
    dump_memory();
    exit(EXIT_SUCCESS);
}

void writer()
{
    signal(SIGTERM, finish_writer);
	signal (SIGINT, SIG_IGN);
    
    while (1){
        dump_memory();
        usleep(5000000);
    }
}
