all: zudiindex

zudiindex: zudiindex/zudiindex zudiindex/zudiindex-addall

zudiindex/zudiindex:
	cd zudiindex; make

install: all
	cd zudiindex; make install

clean: fonyphile
	cd zudiindex; make clean
	rm -f *.o

fonyphile:
	rm -f fonyphile

