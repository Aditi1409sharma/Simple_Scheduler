CC = gcc
CFLAGS = -Wall

programs = ss simpleScheduler hello

all: $(programs)

ss: simpleShell.c shared.c
	$(CC) $(CFLAGS) -o $@ $^

simpleScheduler: simpleScheduler.c shared.c
	$(CC) $(CFLAGS) -o $@ $^

hello: hello.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(programs)
