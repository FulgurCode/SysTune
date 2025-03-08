# Define variables for compiler and flags
CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk4 libadwaita-1)
LDFLAGS = $(shell pkg-config --libs gtk4 libadwaita-1)
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
# The output binary
TARGET = bin/systune
# Installation paths
PREFIX = /usr/
BIN_DIR = $(PREFIX)/bin
ICON_DIR = $(PREFIX)/share/pixmaps
DESKTOP_DIR = $(PREFIX)/share/applications
STYLES_DIR = $(PREFIX)/share/systune/styles
UI_DIR = $(PREFIX)/share/systune/ui

# The default target
all: $(TARGET)
build: $(TARGET)

# Rule to build the target executable
$(TARGET): $(SRCS)
	@mkdir -p $(dir $(TARGET))
	$(CC) $(CFLAGS) -Iinclude -o $@ $(SRCS) $(LDFLAGS)

run: all
	./bin/systune

# Clean up generated files
clean:
	rm -f $(TARGET)

# Install the application
install: all
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(ICON_DIR)
	@mkdir -p $(DESKTOP_DIR)
	@mkdir -p $(STYLES_DIR)
	@mkdir -p $(UI_DIR)
	@cp $(TARGET) $(BIN_DIR)/systune
	@cp assets/systune.png $(ICON_DIR)/systune.png
	@cp systune.desktop $(DESKTOP_DIR)/systune.desktop
	@cp styles/* $(STYLES_DIR)/
	@cp ui/* $(UI_DIR)/
	@chmod +x $(BIN_DIR)/systune
	@echo "SysTune installed successfully."

# Uninstall the application
uninstall:
	@rm -f $(BIN_DIR)/systune
	@rm -f $(ICON_DIR)/systune.png
	@rm -f $(DESKTOP_DIR)/systune.desktop
	@rm -rf /usr/local/share/systune
	@echo "SysTune uninstalled successfully."
