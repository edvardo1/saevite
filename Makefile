CC? = cc
CFLAGS = \
	-Wall -Wextra -Wno-unused -Werror \
	-I./include \
	-ggdb \
	-std=c11 -pedantic \
	$(EXTRA_CFLAGS)
SDLCFLAGS = $(shell pkg-config --cflags sdl3)
SDLLIBS = $(shell pkg-config --libs sdl3)

bin/saevite: src/saevite.c src/saevite_text.c src/*.c include/acyacsl.h bin
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ src/*.c $(SDLLIBS) -lm

obj/stb_truetype.o: src/stb_truetype.c include/stb_truetype.h
	$(CC) $(CFLAGS) -c -o $@ $< -lm

bin:
	mkdir -p bin

obj:
	mkdir -p obj
