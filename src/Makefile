CC=gcc
CFLAGS=-O2 -Wall

OBJS=red.o parser.o simulator.o

all: red_sim

red_sim: $(OBJS)
	$(CC) $(CFLAGS) -o red_sim $(OBJS)

clean:
	rm -f *.o red_sim

