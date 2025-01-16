CC = gcc

CFLAGS = -Wall -Wextra -pedantic -g

SRC = ioctl_test_app.c

TARGET = ioctl_test_app

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
