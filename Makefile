PREFIX?=/usr/local

CFLAGS=-g -Wall 
LDFLAGS=-loauth -lcurl -lm -lpthread -lzmq

all: socsnap

socsnap: bstrlib.o cJSON.o
	cc $(CFLAGS) bstrlib.o cJSON.o socsnap.c -o socsnap $(LDFLAGS) -Wl,-rpath=/usr/local/lib

clean:
	rm -f *.o
	rm -f socsnap
	rm -rf *.dSYM
