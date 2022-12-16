CC=gcc
CFLAGS=-I.

all : server client

server : server.o multModulo.h multModulo.o
	$(CC) -o server -pthread server.o multModulo.o

client : client.o multModulo.h multModulo.o
	$(CC) -o client client.o multModulo.o

server.o : multModulo.h
	$(CC) -o server.o -c -std=c99 server.c $(CFLAGS)

client.o : multModulo.h
	$(CC) -o client.o -c -std=c99 client.c $(CFLAGS)

multModulo.o : multModulo.h
	$(CC) -o multModulo.o -c -std=c99 multModulo.c $(CFLAGS)

clean : 
	rm client.o client server.o server multModulo.o
