# Define variables for compiler and flags
CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk4 libadwaita-1 )
LDFLAGS = $(shell pkg-config --libs gtk4 libadwaita-1)
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)

# The output binary
TARGET = bin/systune

# The source file
SRC = src/main.c

# The default target
all: $(TARGET)
build: $(TARGET)

# Rule to build the target executable
$(TARGET):
	@mkdir -p $(dir $(TARGET))
	$(CC) $(CFLAGS) -Iinclude -o $@ $^ $(LDFLAGS) $(SRCS)

run: all
	@./bin/systune

# Clean up generated files
clean:
	rm -f $(TARGET)
