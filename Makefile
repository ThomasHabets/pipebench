# $Id: Makefile,v 1.2 2002/12/15 19:58:36 marvin Exp $

CC=gcc
CFLAGS=-Wall -w -pedantic

all: pipebench
doc: pipebench.1
install: pipebench
	cp pipebench /usr/local/bin/
	cp pipebench.1 /usr/local/man/man1/

pipebench: pipebench.c
	$(CC) $(CFLAGS) -o pipebench pipebench.c

pipebench.1: pipebench.yodl
	yodl2man -o pipebench.1 pipebench.yodl

clean:
	rm -f pipebench *.o
