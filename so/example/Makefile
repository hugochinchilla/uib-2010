all: ejemplo

ejemplo: ejemplo.o lectores_escritores.o
	gcc -Wall ejemplo.o lectores_escritores.o -o ejemplo

ejemplo.o: ejemplo.c lectores_escritores.h
	gcc -Wall -c ejemplo.c

lectores_escritores.o: lectores_escritores.c lectores_escritores.h
	gcc -Wall -c lectores_escritores.c
