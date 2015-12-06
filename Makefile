all:
	gcc client.c -Wall -lrt -o client
	gcc server.c -Wall -lrt -lpthread -o server


