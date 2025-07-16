# ShiwaPTPTool Makefile
# Copyright (c) 2024 SHIWA NETWORK - All rights reserved
# 
# This software is provided as-is for educational and development purposes.
# Use at your own risk.

CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread -lrt -levent -levent_core -levent_pthreads
QT_LDFLAGS = -lQt5Core -lQt5Widgets -lQt5Gui

# Default target
all: shiwaptptool-cli shiwaptptool-gui

# CLI version
shiwaptptool-cli: src/ptptool_cli.o
	$(CC) $(LDFLAGS) -o $@ $^ 

# GUI version
shiwaptptool-gui: src/ptptool_gui.o
	$(CC) $(QT_LDFLAGS) $(LDFLAGS) -o $@ $^ 

# Object files
src/ptptool_cli.o: src/ptptool_cli.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

src/ptptool_gui.o: src/ptptool_gui.cpp
	$(CC) $(CFLAGS) -fPIC -I/usr/include/qt5 -I/usr/include/qt5/QtCore -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui -o $@ -c $<

# Legacy target for backward compatibility
shiwaptptool: shiwaptptool-cli
	cp shiwaptptool-cli shiwaptptool

# Clean target
.PHONY: clean
clean:
	-rm -f *.o *.log shiwaptptool shiwaptptool-cli shiwaptptool-gui src/*.o

# Format code
format:
	clang-format -i *.cpp src/*.cpp

# Install target
install: all
	sudo cp shiwaptptool-cli /usr/bin/
	sudo cp shiwaptptool-gui /usr/bin/
	sudo chmod +x /usr/bin/shiwaptptool-cli
	sudo chmod +x /usr/bin/shiwaptptool-gui

# Uninstall target
uninstall:
	sudo rm -f /usr/bin/shiwaptptool-cli
	sudo rm -f /usr/bin/shiwaptptool-gui

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build both CLI and GUI versions"
	@echo "  shiwaptptool-cli - Build CLI version only"
	@echo "  shiwaptptool-gui - Build GUI version only"
	@echo "  shiwaptptool     - Build CLI version (legacy)"
	@echo "  clean            - Remove build artifacts"
	@echo "  format           - Format source code"
	@echo "  install          - Install to /usr/bin/"
	@echo "  uninstall        - Remove from /usr/bin/"
	@echo "  help             - Show this help message"
