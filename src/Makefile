CC = gcc
CFLAGS = -g -Wall $(shell pkg-config fuse --cflags) -I$(HOME)/local/include
LDFLAGS = -L$(HOME)/local/lib -lssh -lfuse -Wl,-rpath=$(HOME)/local/lib
SOURCES = myserver.c remote_access.c log.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = myserver

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

log.o: log.c log.h params.h
	$(CC) $(CFLAGS) -c log.c -o log.o

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

