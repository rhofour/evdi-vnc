all: evdi-vnc

evdi-vnc: evdi-vnc.o
	$(CC) evdi-vnc.o -o evdi-vnc -lvncserver

evdi-vnc.o: evdi-vnc.c
	$(CC) -c evdi-vnc.c -o evdi-vnc.o
