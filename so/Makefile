all: agent

agent: lib/lectores_escritores.o lib/server.o lib/client.o lib/writer.o
	gcc -Wall lib/lectores_escritores.o lib/client.o lib/server.o agent.c lib/writer.o -o agent


.PHONY: clean
clean:
	rm -f agent lib/*.o