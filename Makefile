CC?=cc
LD=gcc
WFLAGS=-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wvla -Werror -std=c11 -pedantic
CFLAGS= \
	-I./include \
	$(WFLAGS) \
	$(shell pkg-config --cflags sdl3) \
	$(EXTRACFLAGS)

LIBS=$(shell pkg-config --libs sdl3) -lm

bin/saevite: obj/saevite.o obj/saevite_text.o obj/stb_truetype.o obj/acyacsl.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

obj/saevite.o: src/saevite.c include/acyacsl.h include/gooey.h include/npfont.h include/npunicode.h obj
	$(CC) $(CFLAGS) -c -o $@ $<

obj/saevite_text.o: src/saevite_text.c include/npunicode.h obj
	$(CC) $(CFLAGS) -c -o $@ $<

obj/acyacsl.o: src/acyacsl.c include/acyacsl.h obj
	$(CC) $(CFLAGS) -c -o $@ $<

obj/stb_truetype.o: src/stb_truetype.c include/stb_truetype.h obj
	$(CC) $(CFLAGS) -c -o $@ $<

bin:
	mkdir -p bin

obj:
	mkdir -p obj
