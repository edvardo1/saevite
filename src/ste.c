#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "acyacsl.h"

#include "ste_text.h"

#define GOOEY_IMPLEMENTATION
#include "gooey.h"

/* @todo sandbox this */
#include "stb_truetype.h"
#define NPFONT_IMPLEMENTATION
#include "npfont.h"
#define NPUNICODE_IMPLEMENTATION
#include "npunicode.h"

typedef struct {
	U32 bufferIndex;
} ste_Window;

/* @todo implement tabs later */
typedef struct {
	DynamicArray(ste_Window) windows;
} ste_Tab;

typedef struct {
	gooey_Ctx *gctx;
	npfont_Ctx *fctx;

	DynamicArray(ste_Buffer) buffers;
	DynamicArray(ste_Window) windows;

	gooey_Texture renderTarget;
	Bool drawingNecessary;
	I32 cursor;
} ste_Ste;

void toGlyphs(String8 str, npfont_Glyph **glyphs, U32 *len) {
	*glyphs = malloc(sizeof(npfont_Glyph) * str.len);
	assert(*glyphs != NULL);
	U32 codepoint = 0;
	npfont_Glyph *glyph = NULL;

	npunicode_Utf8Decoder decoder = npunicode_utf8Decoder(str, 0);

	while (npunicode_utf8Decoder_popCodepoint(&decoder, &codepoint) == 0) {
		glyph = &(*glyphs)[*len];
		glyph->codepoints = malloc(sizeof(U32));
		assert(glyph->codepoints != NULL);
		glyph->codepoints[0] = codepoint;
		glyph->len = 1;
		(*len)++;
	}
}

void ste_openWindow(ste_Ste *ste) {
	Allocator npfontAllocator = {malloc, free};
	gooey_Cfg cfg = {0};
	U32 fontIndex = 0;
	npfont_Glyph *glyphs = NULL;
	U32 glyphsLen = 0;

	cfg.w = 800;
	cfg.h = 600;
	cfg.name = "«ste window title»";

	gooey_init(ste->gctx, &cfg);

	npfont_init(&ste->fctx, &npfontAllocator);
	npfont_setGooeyContext(ste->fctx, ste->gctx);

	fontIndex = npfont_loadFont(ste->fctx, "monospace.ttf");
	npfont_pushFont(
		ste->fctx,
		fontIndex,
		npfont_sizeFromPoints(ste->fctx, fontIndex, 30)
	);

	glyphs = NULL;
	toGlyphs(
		S(
			"§ abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"1234567890"
			"!@#$%^&*()`~;:"
			"-=_+[]{}\\|,.<>?/\"'"
		),
		&glyphs,
		&glyphsLen
	);
	npfont_loadGlyphs(ste->fctx, glyphs, glyphsLen);

	gooey_makeTargetableTexture(ste->gctx, &ste->renderTarget, 800, 600);
}

Bool ste_windowShouldClose(ste_Ste *ste) {
	return gooey_windowShouldClose(ste->gctx);
}

void ste_update(ste_Ste *ste) {
	ste_Window *window = &ste->windows.items[0];
	ste_Buffer *buffer = &ste->buffers.items[window->bufferIndex];
	gooey_Event gev = {0};
	U64 key = 0;
	U32 keyMod = 0;
	Bool isDown = false;

	gooey_waitEvent(ste->gctx, &gev);

	if (gooey_event_key_get(ste->gctx, &gev, &key, &keyMod, &isDown) == 0 && isDown) {
		printf("key = %ld\n", key);
		if (key == '\r') {key = '\n';}
		if (key == 8) {
			ste_deleteChar(buffer, ste->cursor - 1);
			ste->cursor -= 1;
			if (ste->cursor < 0) {ste->cursor = 0;}
			ste->drawingNecessary = true;
			ste_printBuffer(buffer);
		//} else if (key == 'q') {
		//	gooey_setWindowShouldClose(ste->gctx, true);
		} else if (key == SDLK_LEFT) {
			/* @todo change this from SDL to gooey */
			ste->cursor -= 1;
			if (ste->cursor < 0) {ste->cursor = 0;}
			ste->drawingNecessary = true;
		} else if (key == SDLK_RIGHT) {
			/* @todo change this from SDL to gooey */
			ste->cursor += 1;
			ste->drawingNecessary = true;
		} else if (key == '\n' || key == ' ' || key == '\t' || (key >= '!' && key <= '~')) {
			ste_insertChar(buffer, ste->cursor, key);
			ste->cursor += 1;
			ste->drawingNecessary = true;
			ste_printBuffer(buffer);
		}
	}
}

void drawCursor(ste_Ste *ste, I32 xPos, I32 yPos, I32 ascent, I32 descent) {
	Rect_I32 cursorRect = rect_I32(
		xPos,
		yPos - ascent,
		5,
		yPos - descent - yPos + ascent
	);

	gooey_setDrawColor(ste->gctx, 0x8888bbff);
	gooey_fillRect(ste->gctx, cursorRect);
}

void ste_renderBuffer(ste_Ste *ste, ste_Buffer *buffer) {
	I32 xPos = 100, yPos = 100, yDiff = 0;
	npfont_GlyphInfo gi = {0};
	npunicode_Utf8Decoder decoder = {0};
	U32 codepoint = 0;
	String8 string = {0};
	Usize pieceIndex = 0;
	Bool isTab;
	I32 defaultAscent = 0, defaultDescent = 0, defaultLineGap = 0;
	npfont_FontSize defaultFontSize = 0;
	I32 defaultYDiff = 0;
	I32 byteCount = 0;

	npfont_getAscent(ste->fctx, -1, &defaultAscent);
	npfont_getDescent(ste->fctx, -1, &defaultDescent);
	npfont_getLineGap(ste->fctx, -1, &defaultLineGap);
	npfont_getFontSize(ste->fctx, -1, &defaultFontSize);
	defaultAscent  *= defaultFontSize;
	defaultDescent *= defaultFontSize;
	defaultLineGap *= defaultFontSize;

	defaultYDiff = defaultAscent - defaultDescent + defaultLineGap;
	for (Usize cpIndex = 0; cpIndex < buffer->currentPieces.len; cpIndex++) {
		pieceIndex = buffer->currentPieces.items[cpIndex];
		string = buffer->allPieces.items[pieceIndex];
		decoder = npunicode_utf8Decoder(string, 0);

		for (
			yDiff = defaultYDiff;
			npunicode_utf8Decoder_popCodepoint(&decoder, &codepoint) == 0;
			byteCount += 1
		) {
			if (codepoint == '\n') {
				if (byteCount == ste->cursor) {
					drawCursor(ste, xPos, yPos, defaultAscent, defaultDescent);
				}
				xPos = 100;
				yPos += yDiff;
				yDiff = defaultYDiff;
				continue;
			}

			isTab = false;
			if (codepoint == '\t') {
				codepoint = ' ';
				isTab = true;
			}

			npfont_getGlyphInfo(ste->fctx, &codepoint, 1, &gi);

			yDiff = MAX(yDiff, (gi.ascent - gi.descent + gi.lineGap) * gi.fontSize);

			if (gi.textureIndex != npfont_NO_TEXTURE) {
				gooey_Texture texture = ste->fctx->textures.items[gi.textureIndex];
				gooey_renderTextureNoSrc(
					ste->gctx,
					texture,
					rect_I32(
						gi.dst.x + xPos,
						gi.dst.y + yPos,
						gi.dst.w,
						gi.dst.h
					)
				);
			}

			if (byteCount == ste->cursor) {
				drawCursor(ste, xPos, yPos, defaultAscent, defaultDescent);
			}

			if (isTab) {
				xPos =
					100 +
					((xPos - 100) / (I32)(gi.xAdvance * 4)) * (I32)(gi.xAdvance * 4) +
					(I32)(gi.xAdvance * 4);
			} else {
				xPos += gi.xAdvance;
			}
		}

		if (byteCount == ste->cursor) {
			drawCursor(ste, xPos, yPos, defaultAscent, defaultDescent);
		}
	}
}

void ste_render(ste_Ste *ste) {
	U64 b = gooey_getNs(ste->gctx);
	ste_Window *window = &ste->windows.items[0];
	ste_Buffer *buffer = &ste->buffers.items[window->bufferIndex];

	if (ste->drawingNecessary) {
		gooey_setRenderTarget(ste->gctx, ste->renderTarget);
		ste->drawingNecessary = false;

		gooey_setDrawColor(ste->gctx, 0xdead00ff);
		gooey_clear(ste->gctx);

		ste_renderBuffer(ste, buffer);

		gooey_setRenderTargetToRoot(ste->gctx);
	}

	gooey_renderTextureNoSrc(
		ste->gctx, ste->renderTarget,
		make_Rect_I32(0, 0, 800, 600)
	);

	U64 e = gooey_getNs(ste->gctx);
	printf("%fms\n", (e - b) / 1000000.0);
}

void ste_closeWindow(ste_Ste *ste) {
	gooey_exit(ste->gctx);
}

void finishTest(String8 test_name, ste_Buffer *buffer, String8 expectedResult) {
	String8 result = {0};
	ste_stringFromBuffer(buffer, &result);
	if (strEq(result, expectedResult)) {
		printf("[OK]    test: %.*s\n", Slens(test_name));
	} else {
		printf("[ERROR] test: %.*s\n", Slens(test_name));
		printf("       got: %ld#«%.*s»\n", result.len, Slens(result));
		printf("  expected: %ld#«%.*s»\n", expectedResult.len, Slens(expectedResult));
	}
}

void test_1() {
	ste_Buffer buffer = {0};
	Uint indices[64];
	Uint i = 0;

	ste_pieceNew(&buffer, S("foo bar baz"), &indices[0]);
	ste_pieceNew(&buffer, S("Hello bro"),   &indices[1]);
	ste_pieceNew(&buffer, S("mi amigo"),   &indices[2]);

	ste_pieceInsert(&buffer, 0, indices[0]);
	ste_pieceInsert(&buffer, 0, indices[1]);
	ste_pieceInsert(&buffer, 0, indices[2]);

	ste_insertChar(&buffer, 2, 'L');
	ste_insertChar(&buffer, 3, 'o');
	ste_insertChar(&buffer, 4, 'r');
	ste_insertChar(&buffer, 5, 'r');
	ste_insertChar(&buffer, 6, 'y');
	ste_deleteChar(&buffer, 10);
	ste_deleteSelection(&buffer, 3, 2);
	ste_deleteSelection(&buffer, 22, 1);
	ste_insertString(&buffer, 22, S("HERE"));

	finishTest(S("1"), &buffer, S("miLry amgoHello brofooHEREbar baz"));
}

void test_2() {
	ste_Buffer buffer = {0};
	String8 result = {0};

	ste_insertChar(&buffer, 0, 'e');
	ste_insertChar(&buffer, 1, 'd');
	ste_insertChar(&buffer, 2, 'o');
	ste_deleteChar(&buffer, 1);

	ste_stringFromBuffer(&buffer, &result);
	finishTest(S("2"), &buffer, S("eo"));
}

void test_3() {
	ste_Buffer buffer = {0};
	String8 result = {0};

	ste_insertChar(&buffer, 0, 'e');
	ste_insertChar(&buffer, 1, 'd');
	ste_insertChar(&buffer, 2, 'o');
	assert(ste_deleteChar(&buffer, 0) == 0);
	assert(ste_deleteChar(&buffer, 0) == 0);
	assert(ste_deleteChar(&buffer, 0) == 0);
	assert(ste_deleteChar(&buffer, 0) != 0);
	assert(ste_deleteChar(&buffer, 0) != 0);

	ste_stringFromBuffer(&buffer, &result);
	finishTest(S("3"), &buffer, S(""));
}

void test_4() {
	ste_Buffer buffer = {0};
	String8 result = {0};

	ste_insertChar(&buffer, 0, 'a');
	ste_insertChar(&buffer, 1, 'b');
	ste_insertChar(&buffer, 2, 'c');
	ste_insertChar(&buffer, 3, 'd');
	ste_insertChar(&buffer, 4, 'e');
	ste_insertChar(&buffer, 5, 'f');
	assert(ste_deleteChar(&buffer, 5) == 0);
	assert(ste_deleteChar(&buffer, 4) == 0);
	ste_insertChar(&buffer, 4, 'E');
	ste_insertChar(&buffer, 5, 'F');

	ste_stringFromBuffer(&buffer, &result);
	finishTest(S("4"), &buffer, S("abcdEF"));
}

int main() {
	gooey_Ctx gctx = {0};
	ste_Ste ste = {0};
	ste.gctx = &gctx;
	ste.drawingNecessary = true;

	test_1();
	test_2();
	test_3();
	test_4();

	//ste_insertChar(&ste.buffer, 0, 'e');
	//ste_insertChar(&ste.buffer, 1, 'd');
	//ste_insertChar(&ste.buffer, 2, 'o');

	daAppendZ(&ste.buffers);
	daAppendZ(&ste.windows);
	ste.windows.items[0].bufferIndex = 0;

	ste_openWindow(&ste);
	while (!ste_windowShouldClose(&ste)) {
		ste_update(&ste);
		ste_render(&ste);
		gooey_present(&gctx);
	}
	ste_closeWindow(&ste);
	
	return 0;
}
