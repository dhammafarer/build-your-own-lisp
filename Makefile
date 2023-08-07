CC=gcc

BUILDDIR = ./build
SRCDIR = ./src
EXECUTABLE = $(BUILDDIR)/lispy

CFLAGS=-std=c99 -Wall -I$(SRCDIR)
LDFLAGS=-ledit

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf ./build
