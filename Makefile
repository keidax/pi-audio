CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -I.

LIBS=-lportaudio -lsndfile -lpthread

DEPS = pi_audio.h master.h
M_OBJ = master.o master_framesync.o common.o
C_OBJ = client.o client_framesync.o common.o

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
