.PHONY: fonyphile clean

CPPFLAGS:=-I../include
CFLAGS:=-O3 -Wall -Wextra -pedantic -std=c99
CXXFLAGS:=-O3 -Wall -Wextra -pedantic -std=c++03
LINK.o=${LINK.cc}
CURR_MAKEFILE_DIR=$(shell pwd)

ifeq (${INSTALL_PREFIX},)
INSTALL_PREFIX:=/usr/bin
endif

ifeq (${ZUI_INCLUDE_DIR},)
$(warning \
This header file is found within the Zambesii kernel source. \
\tSee: include/zui.h \
You will need to download either the whole Zambesii kernel source, \
or just the zui.h header file and provide its location as a parameter \
to the compilation process, using the \"ZUI_INCLUDE_DIR\" environment \
variable. As in, try: \
\tZUI_INCLUDE_DIR=/path/to/zbz/source/include make)
$(warning This must be an absolute path and not a relative one.)
$(error Missing crucial header file zui.h)
endif

all: zudiindex zudiindex-addall

zui.h:
	rm -f zui.h
	ln -s ${ZUI_INCLUDE_DIR}/zui.h zui.h

zui-headers: zui.h

zudiindex: parse.o index.o
zudiindex.o: zudipropsc.h zui-headers
parse.o: zudipropsc.h zui-headers
index.o: zudipropsc.h zui-headers

install: all
	install zudiindex zudiindex-addall ${INSTALL_PREFIX}
	chmod +x ${INSTALL_PREFIX}/zudiindex ${INSTALL_PREFIX}/zudiindex-addall

uninstall:
	rm -f ${INSTALL_PREFIX}/zudiindex ${INSTALL_PREFIX}/zudiindex-addall

clean: fonyphile
	rm -f *.o zudiindex
fonyphile:
	rm -f clean fonyphile

