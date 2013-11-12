.PHONY=fonyphile
CFLAGS=-O3 -Wall -Wextra -pedantic -std=c99
LDFLAGS=-lstdc++
CPPFLAGS=-I../include
all: zudiindex zudiindex-addall

zudiindex: parse.o index.o
zudiindex.o: zudipropsc.h zudiIndex.h
parse.o: zudipropsc.h zudiIndex.h
index.o: zudipropsc.h zudiIndex.h

install: all
	install zudiindex zudiindex-addall /usr/bin
	chmod +x /usr/bin/zudiindex /usr/bin/zudiindex-addall

uninstall:
	rm -f /usr/bin/zudiindex /usr/bin/zudiindex-addall

clean: fonyphile
	rm -f *.o zudiindex
fonyphile:
	rm -f clean fonyphile

