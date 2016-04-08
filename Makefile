# $Id: Makefile,v 1.2 2002/12/15 19:58:36 marvin Exp $

CC?=$(CROSS_COMPILE)gcc
CFLAGS+=-Wall -w -pedantic
PREFIX?=/usr/local

all: pipebench
doc: pipebench.1
install: pipebench
	install pipebench -D $(PREFIX)/bin/pipebench
	install pipebench.1 -D $(PREFIX)/man/man1/pipebench.1

pipebench: pipebench.c
	$(CC) $(CFLAGS) -o pipebench pipebench.c

pipebench.1: pipebench.yodl
	yodl2man -o pipebench.1 pipebench.yodl

clean:
	rm -f pipebench *.o
