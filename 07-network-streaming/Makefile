# Makefile for frame_streamer

CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = frame_streamer
SRC = frame_streamer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
