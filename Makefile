CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC_DIR = src
OBJ_DIR = obj
TARGET = storage_diag_agent

all: $(TARGET)

$(TARGET): $(SRC_DIR)/monitor.c $(SRC_DIR)/monitor.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f /tmp/storage_diag.sock