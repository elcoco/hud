SRC := src
OBJ := obj
LIBS   := -lm -lpthread
PKGCONFIG = $(shell which pkg-config)
CFLAGS = -g -Wall $(shell $(PKGCONFIG) --cflags gtk4)
LDLIBS = $(shell pkg-config --libs gtk4)
CC := cc

$(shell mkdir -p $(OBJ))
NAME := $(shell basename $(shell pwd))

SOURCES := $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: $(OBJECTS)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDLIBS) -o $@ -o $(NAME)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) $(CFLAGS) $(LIBS) $(LDLIBS) -c $< -o $@
