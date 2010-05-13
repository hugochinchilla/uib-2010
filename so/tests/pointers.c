#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int num = 20;
int num2;
int *punt;

int main(){
	//test_pointers();
	demand_memory();
	return 0;
}

int test_pointers(){
	punt = &num;
	printf("Valor vint: %i\n", num);
	printf("Direccion de vint: %i\n", *punt);
	return 0;
}

int demand_memory(){
	int bytes;
	char* texto;

	printf("Cuanta memoria quieres (megas): \n");
	scanf("%i", &bytes);

	texto = (char *) malloc(bytes * 1024 * 1024);
	if (texto) {
		printf("Se reservaron %i megas de memoria en %p\n", bytes, texto);
		sleep(5);
		free(texto);
	} else {
		printf("Pides demasiado 8-)\n");
	}
}
