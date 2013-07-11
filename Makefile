PREFIX?=/usr/local

CFLAGS=-g -Wall 
LDFLAGS=-loauth

all: socsnap

socsnap: 
	cc $(CFLAGS) socsnap.c -o socsnap $(LDFLAGS)

clean:
	rm -f *.o
	rm -f socsnap
	rm -rf *.dSYM
