
# Makefile for CMPSC311 - Assign #1

# Variables
CC=gcc
LINK=gcc
CFLAGS=-c -g -Wall
LINKFLAGS=-L. -g 

# Productions
.SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

assign1-cmpsc311-sp20 : assign1-cmpsc311-sp20.o
	$(LINK) $(LINKFLAGS) assign1-cmpsc311-sp20.o $(LINKLIBS) -o $@

clean : 
	rm -f assign1-cmpsc311-sp20.o assign1-cmpsc311-sp20
