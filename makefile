CC = gcc
CFLAGS = -Wall -std=c99 -I/opt/homebrew/Cellar/sdl2/2.30.7/include
LDFLAGS = -L/opt/homebrew/Cellar/sdl2/2.30.7/lib -lSDL2 -lm
SRCS = ./src/*.c
OUT = renderer

build:
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(OUT)

run:
	./$(OUT)

clean:
	rm -f $(OUT)
