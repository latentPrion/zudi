.PHONY=fonyphile
CFLAGS=-O3 -Wall -Wextra -pedantic -std=c99

all: zudiindex

zudiindex: parse.o index.o
zudiindex.o: zudipropsc.h zudiIndex.h
parse.o: zudipropsc.h zudiIndex.h
index.o: zudipropsc.h zudiIndex.h

install:
	install zudiindex /usr/bin

uninstall:
	rm -f /usr/bin/zudiindex

clean: fonyphile
	rm -f *.o zudiindex
fonyphile:
	rm -f clean fonyphile


