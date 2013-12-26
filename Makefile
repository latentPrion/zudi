all: zudiindex

zudiindex: zudiindex/zudiindex zudiindex/zudiindex-addall

zudiindex/zudiindex:
	cd zudiindex; make

clean: fonyphile
	cd zudiindex; make clean
	rm -f *.o

fonyphile:

