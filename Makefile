CC = gcc
CFLAGS = -Wall

programs = ss simpleScheduler hello

all: $(programs)

ss: simpleShell.c shared.c
	$(CC) $(CFLAGS) -o $@ $^ -lrt

simpleScheduler: simpleScheduler.c shared.c
	$(CC) $(CFLAGS) -o $@ $^ -lrt

hello: hello.c
	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f $(programs)
