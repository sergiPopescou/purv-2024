CC = gcc

CFLAGS = -Wall -Wextra -pedantic -g

SRC = main.c

TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
