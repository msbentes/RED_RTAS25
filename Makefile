CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lm

TARGET = sim
SRC = sim.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
