CC=gcc
CFLAGS=-O -Wall
LDFLAGS=
EXESUFFIX=.exe
RM=rm -f


PACKOBJS= \
	rlepack.o

UNPACKOBJS= \
	rleunpack.o


%.o: %.c
	$(CC) -c $(CFLAGS) $<


all: rlepack rleunpack

rlepack: $(PACKOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

rleunpack: $(UNPACKOBJS)
	$(CC) $(LDFLAGS) -o $@ $^


.PHONY: clean distclean

clean:
	$(RM) rlepack$(EXESUFFIX) rleunpack$(EXESUFFIX) *.o

distclean: clean
	$(RM) *~
