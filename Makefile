# written by Guido Socher
# overwrite with: make prefix=/some/where install
prefix=$(DESTDIR)/usr
INSTALL=install
mandir=$(prefix)/share/man
MANP=man1/whichman.1 man1/ftff.1 man1/ftwhich.1
CC=gcc
CFLAGS= -Wall -O2
#sun c/c++-compiler:
#CC=CC
#CFLAGS= -O

all:whichman ftff ftwhich

whichman: whichman.o levdist.o
	$(CC) -o $@ whichman.o levdist.o

ftwhich: ftwhich.o levdist.o
	$(CC) -o $@ ftwhich.o levdist.o

ftff: ftff.o levdist.o 
	$(CC) -o $@ ftff.o levdist.o

whichman.o: whichman.c 
	$(CC) $(CFLAGS) -c whichman.c
ftwhich.o: ftwhich.c 
	$(CC) $(CFLAGS) -c ftwhich.c
ftff.o: ftff.c 
	$(CC) $(CFLAGS) -c ftff.c
levdist.o: levdist.c levdist.h
	$(CC) $(CFLAGS) -c levdist.c

install: whichman ftff ftwhich $(MANP)
	strip whichman
	strip ftwhich
	strip ftff
	[ -d "$(prefix)/bin" ] || $(INSTALL) -d $(prefix)/bin
	[ -d "$(mandir)/man1" ] || $(INSTALL) -d $(mandir)/man1
	$(INSTALL) -m 755 whichman $(prefix)/bin
	$(INSTALL) -m 755 ftwhich $(prefix)/bin
	$(INSTALL) -m 755 ftff $(prefix)/bin
	for p in $(MANP) ; do \
		echo "installing $$p in $(mandir)/man1"; \
		$(INSTALL) -m 644 $$p $(mandir)/man1 ;\
	done

install_with_cp: whichman ftff ftwhich $(MANP)
	chmod 755 whichman ftff ftwhich
	[ -d "$(prefix)/bin" ] || mkdir -p $(prefix)/bin
	cp whichman ftff ftwhich $(prefix)/bin
	chmod 644 $(MANP)
	[ -d "$(mandir)/man1" ] || mkdir -p $(mandir)/man1
	cp $(MANP) $(mandir)/man1

debug: levdist.c
	$(CC) $(CFLAGS) -o levdebug -DDEBUG -DRETURNVALUE levdist.c

clean:
	rm -f whichman ftff ftwhich *.o levdebug
