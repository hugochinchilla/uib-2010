all: agent

agent: lectores_escritores.o
	gcc -Wall lectores_escritores.o agent.c -o agent

lectores_escritores.o: lectores_escritores.c lectores_escritores.h
	gcc -Wall -c lectores_escritores.c


.PHONY: clean
clean:
	rm -f agent lectores_escritores.o