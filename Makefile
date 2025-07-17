# ShiwaPTPTool Makefile
# Copyright (c) 2024 SHIWA NETWORK - All rights reserved
# 
# This software is provided as-is for educational and development purposes.
# Use at your own risk.

CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread -lrt

# Qt compilation flags from pkg-config
QT_CFLAGS = -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -fPIC -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets -I/usr/include/x86_64-linux-gnu/qt5 -I/usr/include/x86_64-linux-gnu/qt5/QtCore -I/usr/include/x86_64-linux-gnu/qt5/QtGui
QT_LDFLAGS = -lQt5Widgets -lQt5Gui -lQt5Core

# Default target
all: shiwaptptool-cli shiwaptptool-gui

# CLI version
shiwaptptool-cli: src/ptptool_cli.o
	$(CC) $(LDFLAGS) -o $@ $^ 

# GUI version
shiwaptptool-gui: src/ptptool_gui.o
	$(CC) -o $@ $^ $(QT_LDFLAGS) -levent $(LDFLAGS)

# Object files
src/ptptool_cli.o: src/ptptool_cli.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

src/ptptool_gui.moc: src/ptptool_gui.cpp
	moc -o $@ $<

src/ptptool_gui.o: src/ptptool_gui.cpp src/ptptool_gui.moc
	$(CC) $(CFLAGS) $(QT_CFLAGS) -o $@ -c $<

# Legacy target for backward compatibility
shiwaptptool: shiwaptptool-cli
	cp shiwaptptool-cli shiwaptptool

# Clean target
.PHONY: clean
clean:
	-rm -f *.o *.log shiwaptptool shiwaptptool-cli shiwaptptool-gui src/*.o src/*.moc

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
