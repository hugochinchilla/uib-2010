#define _USE_BSD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "lectores_escritores.h"

#define OPS	1000
#define USECS	10000
#define PROCS	10

int acabados = 0;

void reaper(int s)
{
	while ((wait3(NULL, WNOHANG, NULL)) > 0) acabados++;
	printf("Acabaron %d hijos\n",acabados);
}

int main()
{
	int i, pid, op;

	signal(SIGCHLD, reaper);
	setlinebuf(stdout);
	inicializar_le();
	pid = getpid();
	for (i=0; i<PROCS; i++) {
		if (fork() == 0) { // Hijos
			pid = getpid();
			srandom(pid);
			for (i=0; i<OPS; i++) {
				op = random() % 5;
				if (op) { // Lector
					printf("Voy a leer (%d)\n", pid);
					entrada_lectores();
					printf("Estoy leyendo (%d)\n", pid);
					usleep(USECS);
					printf("Acabo de leer (%d)\n", pid);
					salida_lectores();
				} else { // Escritor
					printf("Voy a escribir (%d)\n", pid);
					entrada_escritores();
					printf("Estoy escribiendo (%d)\n", pid);
					usleep(USECS>>2); // Se desplazan 2 bits de izquierda a derecha
					printf("Acabo de escribir (%d)\n", pid);
					salida_escritores();
				}
			}
			printf("Acabado (%d)\n", pid);
			exit(0);
		}
	}
	while (acabados < PROCS) pause();
	eliminar_le();
	return 0;
}
