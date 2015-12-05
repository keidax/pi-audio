CC=gcc
CFLAGS=-I.

LIBS=-lportaudio

DEPS = test.h
OBJ = test.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

pi-audio: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o 
