.PHONY=fonyphile
CFLAGS=-O3 -Wall -Wextra -pedantic -std=c99

all: zudipropsc

zudipropsc: parse.o
zudipropsc.o: zudipropsc.h zudiIndex.h
parse.o: zudipropsc.h zudiIndex.h

install:
	install zudipropsc /usr/bin

uninstall:
	rm -f /usr/bin/zudipropsc

clean: fonyphile
	rm -f *.o zudipropsc
fonyphile:
	rm -f clean fonyphile


