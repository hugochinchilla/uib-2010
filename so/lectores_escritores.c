#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "lectores_escritores.h"

static int lectores_escritores;
static int mutex;

int nuevo_sem(int llave, int num)
{
	int s;

	s = semget(llave, num, IPC_CREAT|0600);
	if (s < 0) {
		puts("Error creando semáforos");
		exit(0);
	}
	return s;
}

int esperar_sem(int s, int pos, int flag)
{
	struct sembuf sbuf;

	sbuf.sem_num = pos;
	sbuf.sem_op = -1;
	sbuf.sem_flg = flag;
	return semop(s, &sbuf, 1);
}

int senalizar_sem(int s, int pos)
{
	struct sembuf sbuf;

	sbuf.sem_num = pos;
	sbuf.sem_op = 1;
	sbuf.sem_flg = 0;
	return semop(s, &sbuf, 1);
}

int esperar_cero(int s, int pos)
{
	struct sembuf sbuf;

	sbuf.sem_num = pos;
	sbuf.sem_op = 0;
	sbuf.sem_flg = 0;
	return semop(s, &sbuf, 1);
}

int eliminar_sem(int s)
{
        return semctl(s, 0, IPC_RMID, 0);
}

void inicializar_le()
{
	int clave = 50; // Valor 50 sólo como ejemplo (sin ftok)
	int sarg_array[] = {0, 0};
	int sarg = 1;

	lectores_escritores = nuevo_sem(clave, 2);
	// [0] = escritores
	// [1] = lectores
	semctl(lectores_escritores, 0, SETALL, sarg_array);

	mutex = nuevo_sem(clave+1, 1);
	semctl(mutex, 0, SETVAL, sarg);
}

void entrada_lectores()
{
	struct sembuf sbuf[2];
	// [0] = escritores
	// [1] = lectores

	// Wait-for-zero escritores
	sbuf[0].sem_num = 0;
	sbuf[0].sem_op = 0;
	sbuf[0].sem_flg = 0;

	// Señalizar lectores
	sbuf[1].sem_num = 1;
	sbuf[1].sem_op = 1;
	sbuf[1].sem_flg = 0;

	semop(lectores_escritores, sbuf, 2);
}

void salida_lectores()
{
	esperar_sem(lectores_escritores, 1, 0);
}

void entrada_escritores()
{
	// Señalizar escritores
	senalizar_sem(lectores_escritores, 0);
	// Esperar 0 de lectores
	esperar_cero(lectores_escritores, 1);
	esperar_sem(mutex, 0, 0);
}

void salida_escritores()
{
	senalizar_sem(mutex, 0);
	esperar_sem(lectores_escritores, 0, 0);
}

void eliminar_le()
{
	eliminar_sem(mutex);
	eliminar_sem(lectores_escritores);	
}
