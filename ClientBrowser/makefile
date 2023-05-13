CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = httpServer

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET)
