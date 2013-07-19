PREFIX?=/usr/local

CFLAGS=-g -Wall 
LDFLAGS=-loauth

all: socsnap

socsnap: bstrlib.o
	cc $(CFLAGS) bstrlib.o socsnap.c -o socsnap $(LDFLAGS)

clean:
	rm -f *.o
	rm -f socsnap
	rm -rf *.dSYM
