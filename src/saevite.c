#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "acyacsl.h"

#include "saevite_text.h"

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
} saevite_Window;

/* @todo implement tabs later */
typedef struct {
	DynamicArray(saevite_Window) windows;
} saevite_Tab;

typedef struct {
	gooey_Ctx *gctx;
	npfont_Ctx *fctx;

	DynamicArray(saevite_Buffer) buffers;
	DynamicArray(saevite_Window) windows;

	gooey_Texture renderTarget;
	Bool drawingNecessary;
} saevite_Saevite;

Void toGlyphs(String8 str, npfont_Glyph **glyphs, U32 *len) {
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

Void saevite_openWindow(saevite_Saevite *saevite) {
	Allocator npfontAllocator = {malloc, free};
	gooey_Cfg cfg = {0};
	U32 fontIndex = 0;
	npfont_Glyph *glyphs = NULL;
	U32 glyphsLen = 0;

	cfg.w = 800;
	cfg.h = 600;
	cfg.name = "«saevite window title»";

	gooey_init(saevite->gctx, &cfg);

	npfont_init(&saevite->fctx, &npfontAllocator);
	npfont_setGooeyContext(saevite->fctx, saevite->gctx);

	fontIndex = npfont_loadFont(saevite->fctx, "monospace.ttf");
	npfont_pushFont(
		saevite->fctx,
		fontIndex,
		npfont_sizeFromPoints(saevite->fctx, fontIndex, 30)
	);

	glyphs = NULL;
	toGlyphs(
		S(
			"§«» abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"1234567890"
			"!@#$%^&*()`~;:"
			"-=_+[]{}\\|,.<>?/\"'"
		),
		&glyphs,
		&glyphsLen
	);
	npfont_loadGlyphs(saevite->fctx, glyphs, glyphsLen);

	gooey_makeTargetableTexture(saevite->gctx, &saevite->renderTarget, 800, 600);
}

Bool saevite_windowShouldClose(saevite_Saevite *saevite) {
	return gooey_windowShouldClose(saevite->gctx);
}

void breakfun(void) {}

Void saevite_update(saevite_Saevite *saevite) {
	saevite_Window *window = &saevite->windows.items[0];
	saevite_Buffer *buffer = &saevite->buffers.items[window->bufferIndex];
	gooey_Event gev = {0};
	U64 key = 0;
	U32 keyMod = 0;
	Bool isDown = false;
	Int *cursor = NULL;

	assert(buffer->cursors.len > 0);
	cursor = &buffer->cursors.items[buffer->cursors.len - 1].position;

	gooey_waitEvent(saevite->gctx, &gev);

	if (gooey_event_key_get(saevite->gctx, &gev, &key, &keyMod, &isDown) == 0 && isDown) {
		printf("key = %ld, keymod = %x\n", key, keyMod);
		if (key == '\r') {key = '\n';}

		if (!!(keyMod & gooey_KEYMOD_LCTRL) && key == 'z') {
			saevite_undo(buffer);
			saevite->drawingNecessary = true;

			saevite_printBuffer(buffer);
			printf("cursor: %d\n", *cursor);
		} else if (!!(keyMod & gooey_KEYMOD_LCTRL) && key == 'y') {
			saevite_redo(buffer);
			saevite->drawingNecessary = true;

			saevite_printBuffer(buffer);
			printf("cursor: %d\n", *cursor);
		} else if (key == 8) {
			saevite_deleteChar(buffer, 0, *cursor - 1);
			*cursor -= 1;
			if (*cursor < 0) {*cursor = 0;}
			saevite->drawingNecessary = true;

			saevite_printBuffer(buffer);
			printf("cursor: %d\n", *cursor);
		//} else if (key == 'q') {
		//	gooey_setWindowShouldClose(saevite->gctx, true);
		} else if (key == SDLK_LEFT) {
			/* @todo change this from SDL to gooey */
			saevite_buffer_addUndoMarkerIfNecessary(buffer);
			*cursor -= 1;
			if (*cursor < 0) {*cursor = 0;}
			saevite->drawingNecessary = true;
		} else if (key == SDLK_RIGHT) {
			/* @todo change this from SDL to gooey */
			saevite_buffer_addUndoMarkerIfNecessary(buffer);
			*cursor += 1;
			saevite->drawingNecessary = true;
		} else if (key == '\n' || key == ' ' || key == '\t' || (key >= '!' && key <= '~')) {
			//if (key == '\n') {
			//	buffer->doMergeInsertedChars = false;
			//	buffer->mode = saevite_BufferMode_None;
			//} else {
			//	buffer->doMergeInsertedChars = true;
			//}
			//buffer->doMergeInsertedChars = true;
			if (key == '\n') {
				saevite_buffer_addUndoMarkerIfNecessary(buffer);
				saevite_insertChar(buffer, 0, *cursor, key);
				saevite_buffer_addUndoMarkerIfNecessary(buffer);
			} else {
				if (key == 'r') {
					saevite_printBuffer(buffer);
					breakfun();
				}
				saevite_insertChar(buffer, 0, *cursor, key);
			}

			*cursor += 1;
			saevite->drawingNecessary = true;

			saevite_printBuffer(buffer);
			printf("cursor: %d\n", *cursor);
		}
	}
}

Void drawCursor(saevite_Saevite *saevite, I32 xPos, I32 yPos, I32 ascent, I32 descent) {
	Rect_I32 cursorRect = rect_I32(
		xPos,
		yPos - ascent,
		5,
		yPos - descent - yPos + ascent
	);

	gooey_setDrawColor(saevite->gctx, 0x8888bbff);
	gooey_fillRect(saevite->gctx, cursorRect);
}

Void saevite_renderBuffer(saevite_Saevite *saevite, saevite_Buffer *buffer) {
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
	Int cursorPosition = 0;

	assert(buffer->cursors.len > 0);
	cursorPosition = buffer->cursors.items[buffer->cursors.len - 1].position;

	npfont_getAscent(saevite->fctx, -1, &defaultAscent);
	npfont_getDescent(saevite->fctx, -1, &defaultDescent);
	npfont_getLineGap(saevite->fctx, -1, &defaultLineGap);
	npfont_getFontSize(saevite->fctx, -1, &defaultFontSize);
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
				if (byteCount == cursorPosition) {
					drawCursor(saevite, xPos, yPos, defaultAscent, defaultDescent);
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

			npfont_getGlyphInfo(saevite->fctx, &codepoint, 1, &gi);

			yDiff = MAX(yDiff, (gi.ascent - gi.descent + gi.lineGap) * gi.fontSize);

			if (gi.textureIndex != npfont_NO_TEXTURE) {
				gooey_Texture texture = saevite->fctx->textures.items[gi.textureIndex];
				gooey_renderTextureNoSrc(
					saevite->gctx,
					texture,
					rect_I32(
						gi.dst.x + xPos,
						gi.dst.y + yPos,
						gi.dst.w,
						gi.dst.h
					)
				);
			}

			if (byteCount == cursorPosition) {
				drawCursor(saevite, xPos, yPos, defaultAscent, defaultDescent);
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

		if (byteCount == cursorPosition) {
			drawCursor(saevite, xPos, yPos, defaultAscent, defaultDescent);
		}
	}
	if (buffer->currentPieces.len == 0) {
		drawCursor(saevite, xPos, yPos, defaultAscent, defaultDescent);
	}
}

Void saevite_render(saevite_Saevite *saevite) {
	U64 b = gooey_getNs(saevite->gctx);
	saevite_Window *window = &saevite->windows.items[0];
	saevite_Buffer *buffer = &saevite->buffers.items[window->bufferIndex];

	if (saevite->drawingNecessary) {
		gooey_setRenderTarget(saevite->gctx, saevite->renderTarget);
		saevite->drawingNecessary = false;

		gooey_setDrawColor(saevite->gctx, 0xdead00ff);
		gooey_clear(saevite->gctx);

		saevite_renderBuffer(saevite, buffer);

		gooey_setRenderTargetToRoot(saevite->gctx);
	}

	gooey_renderTextureNoSrc(
		saevite->gctx, saevite->renderTarget,
		make_Rect_I32(0, 0, 800, 600)
	);

	U64 e = gooey_getNs(saevite->gctx);
	UNUSED(b);
	UNUSED(e);
	//printf("%ldus\n", (e - b) / 1000);
}

Void saevite_closeWindow(saevite_Saevite *saevite) {
	gooey_exit(saevite->gctx);
}

Void finishTest(String8 test_name, saevite_Buffer *buffer, String8 expectedResult) {
	String8 result = {0};
	saevite_stringFromBuffer(buffer, &result);
	if (strEq(result, expectedResult)) {
		printf("[OK]    test: %.*s\n", Slens(test_name));
	} else {
		printf("[ERROR] test: %.*s\n", Slens(test_name));
		printf("       got: %ld#«%.*s»\n", result.len, Slens(result));
		printf("  expected: %ld#«%.*s»\n", expectedResult.len, Slens(expectedResult));
	}
}

Void test_1(Void) {
	saevite_Buffer buffer = {0};

	saevite_buffer_init(&buffer);

	saevite_insertString(&buffer, 0, S("foo bar baz"));
	saevite_insertString(&buffer, 3 + 1 + 3 + 1 + 3, S("Hello bro"));
	saevite_insertString(&buffer, 3 * 3 + 2 + 5 + 1 + 3, S("mi amigo"));

	saevite_insertChar(&buffer, 0, 2, 'L');
	saevite_insertChar(&buffer, 0, 3, 'o');
	saevite_insertChar(&buffer, 0, 4, 'r');
	saevite_insertChar(&buffer, 0, 5, 'r');
	saevite_insertChar(&buffer, 0, 6, 'y');
	saevite_deleteChar(&buffer, 0, 10);
	saevite_deleteSelection(&buffer, 3, 2);
	saevite_deleteSelection(&buffer, 22, 1);
	saevite_insertString(&buffer, 22, S("HERE"));

	finishTest(S("1"), &buffer, S("foLryo br bazHello broHEREi amigo"));
}

Void test_2(Void) {
	saevite_Buffer buffer = {0};
	String8 result = {0};

	saevite_buffer_init(&buffer);

	saevite_insertChar(&buffer, 0, 0, 'e');
	saevite_insertChar(&buffer, 0, 1, 'd');
	saevite_insertChar(&buffer, 0, 2, 'o');
	saevite_deleteChar(&buffer, 0, 1);

	saevite_stringFromBuffer(&buffer, &result);
	finishTest(S("2"), &buffer, S("eo"));
}

Void test_3(Void) {
	saevite_Buffer buffer = {0};
	String8 result = {0};

	saevite_buffer_init(&buffer);

	saevite_insertChar(&buffer, 0, 0, 'e');
	saevite_insertChar(&buffer, 0, 1, 'd');
	saevite_insertChar(&buffer, 0, 2, 'o');
	assert(saevite_deleteChar(&buffer, 0, 0) == 0);
	assert(saevite_deleteChar(&buffer, 0, 0) == 0);
	assert(saevite_deleteChar(&buffer, 0, 0) == 0);
	assert(saevite_deleteChar(&buffer, 0, 0) != 0);
	assert(saevite_deleteChar(&buffer, 0, 0) != 0);

	saevite_stringFromBuffer(&buffer, &result);
	finishTest(S("3"), &buffer, S(""));
}

Void test_4(Void) {
	saevite_Buffer buffer = {0};
	String8 result = {0};

	saevite_buffer_init(&buffer);

	saevite_insertChar(&buffer, 0, 0, 'a');
	saevite_insertChar(&buffer, 0, 1, 'b');
	saevite_insertChar(&buffer, 0, 2, 'c');
	saevite_insertChar(&buffer, 0, 3, 'd');
	saevite_insertChar(&buffer, 0, 4, 'e');
	saevite_insertChar(&buffer, 0, 5, 'f');
	assert(saevite_deleteChar(&buffer, 0, 5) == 0);
	assert(saevite_deleteChar(&buffer, 0, 4) == 0);
	saevite_insertChar(&buffer, 0, 4, 'E');
	saevite_insertChar(&buffer, 0, 5, 'F');

	saevite_stringFromBuffer(&buffer, &result);
	finishTest(S("4"), &buffer, S("abcdEF"));
}

Void test_5(Void) {
	saevite_Buffer buffer = {0};
	String8 result = {0};

	saevite_buffer_init(&buffer);

	saevite_insertChar(&buffer, 0, 0, 'a');
	saevite_insertChar(&buffer, 0, 1, 'b');
	saevite_insertChar(&buffer, 0, 2, 'c');

	saevite_buffer_addUndoMarkerIfNecessary(&buffer);
	saevite_insertChar(&buffer, 0, 3, '\n');
	saevite_buffer_addUndoMarkerIfNecessary(&buffer);

	saevite_insertChar(&buffer, 0, 4, 'd');
	saevite_insertChar(&buffer, 0, 5, 'e');
	saevite_insertChar(&buffer, 0, 6, 'f');

	saevite_undo(&buffer);

	saevite_stringFromBuffer(&buffer, &result);
	finishTest(S("5"), &buffer, S("abc\n"));
}

Int main(Void) {
	gooey_Ctx gctx = {0};
	saevite_Saevite saevite = {0};
	saevite.gctx = &gctx;
	saevite.drawingNecessary = true;

	test_1();
	test_2();
	test_3();
	test_4();
	test_5();

	daAppendZ(&saevite.buffers);
	saevite_buffer_init(&saevite.buffers.items[saevite.buffers.len - 1]);
	daAppendZ(&saevite.windows);
	saevite.windows.items[0].bufferIndex = 0;

	saevite_openWindow(&saevite);
	while (!saevite_windowShouldClose(&saevite)) {
		saevite_update(&saevite);
		saevite_render(&saevite);
		gooey_present(&gctx);
	}
	saevite_closeWindow(&saevite);
	
	return 0;
}
