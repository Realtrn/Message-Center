compile:
	gcc -Wall -g3 -fsanitize=address -pthread server.c -o server.o
	gcc -Wall -g3 -fsanitize=address -pthread client.c -o client.o
FLAGS    = -L /lib64
LIBS     = -lusb-1.0 -l pthread

