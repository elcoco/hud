SRC := src
#VPATH = src:src/modules/apps
OBJ := obj
LIBS   := -lm -lpthread
PKGCONFIG = $(shell which pkg-config)
CFLAGS = -g -Wall $(shell $(PKGCONFIG) --cflags gtk4 --libs dbus-1)
LDLIBS = $(shell pkg-config --libs gtk4)
CC := cc

$(shell mkdir -p $(OBJ))
NAME := $(shell basename $(shell pwd))

SOURCES := $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/*.c) 
#SOURCES=$(shell find src -type f -iname '*.c')
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: $(OBJECTS)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDLIBS) -o $@ -o $(NAME)

$(OBJ)/%.o: $(SRC)/%.c
	glib-compile-resources resources/gresource.xml --target=src/resources.c --generate-source
	$(CC) -I$(SRC) $(CFLAGS) $(LIBS) $(LDLIBS) -c $< -o $@

gen:
	gdbus-codegen --generate-c-code mpris_gdbus --output-directory src --c-namespace mpris --interface-prefix org.mpris. src/org.mpris.MediaPlayer2.xml
