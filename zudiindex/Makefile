.PHONY: fonyphile clean

CPPFLAGS:=-I../include
CFLAGS:=-O3 -Wall -Wextra -pedantic -std=c99
CXXFLAGS:=-O3 -Wall -Wextra -pedantic -std=c++03
LINK.o=${LINK.cc}

ifeq (${INSTALL_PREFIX},)
INSTALL_PREFIX:=/usr/bin
endif

all: zudiindex zudiindex-addall

zudiindex: parse.o index.o
zudiindex.o: zudipropsc.h zui.h
parse.o: zudipropsc.h zui.h
index.o: zudipropsc.h zui.h

install: all
	install zudiindex zudiindex-addall ${INSTALL_PREFIX}
	chmod +x ${INSTALL_PREFIX}/zudiindex ${INSTALL_PREFIX}/zudiindex-addall

uninstall:
	rm -f ${INSTALL_PREFIX}/zudiindex ${INSTALL_PREFIX}/zudiindex-addall

clean: fonyphile
	rm -f *.o zudiindex
fonyphile:
	rm -f clean fonyphile

