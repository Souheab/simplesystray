CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0)

TARGET = simplesystray
SOURCE = simplesystray-gui.c simplesystray-dbus-client.c dbus-status-notifier-watcher.c 

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean debug
