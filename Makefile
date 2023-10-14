CC = gcc
CFLAGS = -Wall

programs = ss hello

all: $(programs)

ss: simpleShell.c
	$(CC) $(CFLAGS) -o $@ $<

hello: hello.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(programs)
