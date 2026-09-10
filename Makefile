CC      = gcc
CFLAGS  = -Wall -Wextra -std=gnu11
SRC_DIR = src
SRCS    = $(wildcard $(SRC_DIR)/*.c)
OBJS    = $(SRCS:.c=.o)
TARGET  = mishell

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
