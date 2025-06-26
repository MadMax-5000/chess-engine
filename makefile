CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Wno-unused-parameter

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf

INC_DIRS = -I.
SRC_FILES = main.c board.c sdl_graphics.c rules.c ai.c # Added ai.c
OBJ_FILES = $(SRC_FILES:.c=.o)
TARGET = chess_engine

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET) $(SDL_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INC_DIRS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET) $(TARGET).exe

run: all
	./$(TARGET)

.PHONY: all clean run