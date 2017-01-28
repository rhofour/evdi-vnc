# Set reasonable defaults
CC ?= clang
CFLAGS ?= -Wall \
	-D_FORTIFY_SOURCE=2 \
	-Wextra -Wcast-align -Wcast-qual -Wpointer-arith \
	-Waggregate-return -Wunreachable-code -Wfloat-equal \
	-Wformat=2 -Wredundant-decls -Wundef \
	-Wdisabled-optimization -Wshadow -Wmissing-braces \
	-Wstrict-aliasing=2 -Wstrict-overflow=5 -Wconversion \
	-Wno-unused-parameter \
	-pedantic -std=c11 -g -O2

all: evdi-vnc

evdi-vnc: evdi-vnc.o evdi_lib
	$(CC) evdi-vnc.o ./evdi/library/evdi_lib.o -o evdi-vnc -lvncserver 

evdi-vnc.o: evdi-vnc.c
	$(CC) $(CFLAGS) -c evdi-vnc.c -o evdi-vnc.o -Ievdi/library/

evdi_lib:
	@$(MAKE) -C evdi/library

clean:
	rm -f evdi-vnc *.o
