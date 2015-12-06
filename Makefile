CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -I.

LIBS=-lportaudio -lsndfile

DEPS = master.h
M_OBJ = master.c
C_OBJ = pi-client.c

all: pi-audio-client pi-audio-master


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

pi-audio-master: $(M_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pi-audio-client: $(C_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o pi-audio-master pi-audio-client
