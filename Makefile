PREFIX?=/usr/local

CFLAGS=-g -Wall 
LDFLAGS=-loauth -lcurl -lm -lpthread -lzmq

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=bin/socsnap

all: $(TARGET)

$(TARGET): build $(OBJECTS) 
	cc $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS) -Wl,-rpath=/usr/local/lib

build: 
	@mkdir -p build
	@mkdir -p bin

clean:
	rm -f $(OBJECTS) 
	rm -f $(TARGET)
