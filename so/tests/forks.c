#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int main()
{
    int i,pid,child,sem;
    key_t sem_key;
    
    pid = getpid();
    printf("Soy el padre y mi PID es %d\n", pid);
    
    sem_key = ftok("/bin/ls", 42);
    sem = semget(sem_key, 1, IPC_CREAT|0600);
    if (sem < 0) {
        perror("semget");
        exit(1);
    }
    
    
    for (i=0; i<10; i++) {
        child = fork();
        
        if (child == -1) {
            perror("fork");
        }
        if (child == 0) { //CHILD
            pid = getpid();
            printf("Soy un hijo y mi PID es %d\n", pid);
            signal_sem(sem);
            exit(0);
        } else { //PARENT
            wait_sem(sem);
            printf("Continuacion del padre\n", pid);
        }
    }
    
    exit(0);
}

int wait_sem(int s)
{
	struct sembuf sbuf;

	sbuf.sem_num = 0;
	sbuf.sem_op = -1;
	sbuf.sem_flg = 0;
	return semop(s, &sbuf, 1);
}

int signal_sem(int s)
{
	struct sembuf sbuf;

	sbuf.sem_num = 0;
	sbuf.sem_op = 1;
	sbuf.sem_flg = 0;
	return semop(s, &sbuf, 1);
}
