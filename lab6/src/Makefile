CC=g++
CFLAGS=-I.
all : client server

client : 
	$(CC) -o client client.cpp -lpthread $(CFLAGS)
	
server: 
	$(CC) -o server server.cpp -lpthread $(CFLAGS)

clear:
	rm -f server client
	
