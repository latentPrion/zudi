all: zudiindex

zudiindex: zudiindex/zudiindex zudiindex/zudiindex-addall

zudiindex/zudiindex:
	cd zudiindex; INSTALL_PREFIX=${INSTALL_PREFIX} $(MAKE)

install: all
	cd zudiindex; $(MAKE) install

uninstall:
	cd zudiindex; $(MAKE) uninstall

clean: fonyphile
	cd zudiindex; $(MAKE) clean
	rm -f *.o

fonyphile:
	rm -f fonyphile

