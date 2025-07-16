# PTP Tool Makefile
# Copyright (c) 2024 [Your Name] - All rights reserved
# 
# This software is provided as-is for educational and development purposes.
# Use at your own risk.

CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread -lrt -levent -levent_core -levent_pthreads
QT_LDFLAGS = -lQt5Core -lQt5Widgets -lQt5Gui

# Default target
all: ptptool-cli ptptool-gui

# CLI version
ptptool-cli: src/ptptool_cli.o
	$(CC) $(LDFLAGS) -o $@ $^ 

# GUI version
ptptool-gui: src/ptptool_gui.o
	$(CC) $(QT_LDFLAGS) $(LDFLAGS) -o $@ $^ 

# Object files
src/ptptool_cli.o: src/ptptool_cli.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

src/ptptool_gui.o: src/ptptool_gui.cpp
	$(CC) $(CFLAGS) -fPIC -I/usr/include/qt5 -I/usr/include/qt5/QtCore -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui -o $@ -c $<

# Legacy target for backward compatibility
ptptool: ptptool-cli
	cp ptptool-cli ptptool

# Clean target
.PHONY: clean
clean:
	-rm -f *.o *.log ptptool ptptool-cli ptptool-gui src/*.o

# Format code
format:
	clang-format -i *.cpp src/*.cpp

# Install target
install: all
	sudo cp ptptool-cli /usr/bin/
	sudo cp ptptool-gui /usr/bin/
	sudo chmod +x /usr/bin/ptptool-cli
	sudo chmod +x /usr/bin/ptptool-gui

# Uninstall target
uninstall:
	sudo rm -f /usr/bin/ptptool-cli
	sudo rm -f /usr/bin/ptptool-gui

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build both CLI and GUI versions"
	@echo "  ptptool-cli  - Build CLI version only"
	@echo "  ptptool-gui  - Build GUI version only"
	@echo "  ptptool      - Build CLI version (legacy)"
	@echo "  clean        - Remove build artifacts"
	@echo "  format       - Format source code"
	@echo "  install      - Install to /usr/bin/"
	@echo "  uninstall    - Remove from /usr/bin/"
	@echo "  help         - Show this help message"
