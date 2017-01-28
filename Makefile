all: evdi-vnc

evdi-vnc: evdi-vnc.o evdi_lib
	$(CC) evdi-vnc.o ./evdi/library/evdi_lib.o -o evdi-vnc -lvncserver 

evdi-vnc.o: evdi-vnc.c
	$(CC) -Wall -g -c evdi-vnc.c -o evdi-vnc.o -Ievdi/library/

evdi_lib:
	@$(MAKE) -C evdi/library

clean:
	rm -f evdi-vnc *.o
