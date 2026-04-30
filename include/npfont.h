#if !defined(NPFONT_H)
#define NPFONT_H

#define npfont_NO_FONT    ((U32)0xffffffff)
#define npfont_NO_TEXTURE ((U16)0xffff)
//#define npfont_NO_SIZE (~(U32)0)

typedef struct npfont_Ctx npfont_Ctx;

typedef U32 npfont_FontIndex;
typedef float npfont_FontSize;

typedef struct npfont_Glyph npfont_Glyph;
typedef struct npfont_GlyphInfo npfont_GlyphInfo;

struct npfont_GlyphInfo {
	Rect_I32 src;
	Rect_I32 dst;
	I32 textureIndex;
	I32 xAdvance;
	I32 xOffset;
	I32 yOffset;
	I32 ascent;
	I32 descent;
	I32 lineGap;
	npfont_FontSize fontSize;
};

/* Q: why is this struct like this instead of just a U32
 * A: to support multiple codepoint glyphs in the future :) */
struct npfont_Glyph {
	U32 *codepoints;
	U32 len;
};

void npfont_init(npfont_Ctx **ctxPtr, Allocator *allocator);
void npfont_deinit(npfont_Ctx *ctx);

npfont_FontSize npfont_sizeFromPoints(
	npfont_Ctx *ctx,
	const npfont_FontIndex fontIndex,
	const I32 points
);
void npfont_setGooeyContext(npfont_Ctx *ctx, gooey_Ctx *gctx);

npfont_FontIndex npfont_loadFont(npfont_Ctx *ctx, const char *filepath);

void npfont_pushFont(npfont_Ctx *ctx, npfont_FontIndex index, npfont_FontSize size);
void npfont_popFont(npfont_Ctx *ctx, npfont_FontIndex *index, npfont_FontSize *size);
void npfont_peekFont(npfont_Ctx *ctx, npfont_FontIndex *index, npfont_FontSize *size);

void npfont_loadGlyphs(npfont_Ctx *ctx, npfont_Glyph *glyphs, U32 glyphsLen);
Int npfont_getGlyphInfo(
	npfont_Ctx *ctx,
	U32 *codepoints,
	U32 codepointsLen,
	npfont_GlyphInfo *glyphInfo
);

#endif /* !defined(NPFONT_H) */

#if defined(NPFONT_IMPLEMENTATION)

typedef struct npfont_MapKey npfont_MapKey;
typedef struct npfont_MapValue npfont_MapValue;

#define TOMBSTONE_FONTINDEX 0xffffffff
static_assert(sizeof(npfont_FontIndex) == 4);

struct npfont_MapKey {
	npfont_Glyph glyph;
	npfont_FontIndex fontIndex;
	npfont_FontSize fontSize;
};

struct npfont_MapValue {
	Rect_I32 src;

	I32 xOffset;
	I32 yOffset;
	I32 xAdvance;

	I32 textureIndex;
	U16 width;
	U16 height;
};

#define FONT_STACK_CAPACITY 32

struct npfont_Ctx {
	npfont_MapKey *mapKeys;
	npfont_MapValue *mapValues;
	U32 mapLen;
	U32 mapCount;

	struct {
		npfont_FontSize  size;
		npfont_FontIndex index;
	} fontStack[FONT_STACK_CAPACITY];
	I32 fontStackTop;

	//npfont_FontSize currentSize;
	//npfont_FontIndex currentFontIndex;
	/*String8 currentText;*/
	/*Rect_I32 lastDimensions;*/

	/* maybe try keeping the bitmaps as well */
	genDa(gooey_Texture) textures;
	Allocator allocator;
	Arena codepointsArena;
	Arena pixelsArena;

	gooey_Ctx *gctx;

	/* sbtt stuff */
	genDa(stbtt_fontinfo) fonts;
	genDa(U8 *) fontFiles;
};

U32 npfont__hashKey(const npfont_MapKey *key) {
	return 
		djb2(key->glyph.codepoints, key->glyph.len * sizeof(*key->glyph.codepoints)) ^
		djb2(&key->fontIndex, sizeof(key->fontIndex)) ^
		djb2(&key->fontSize, sizeof(key->fontSize));
}

void npfont__growMap(npfont_Ctx *ctx) {
	npfont_MapKey *oldKeys = ctx->mapKeys;
	npfont_MapValue *oldValues = ctx->mapValues;
	npfont_MapKey *newKeys = NULL;
	npfont_MapValue *newValues = NULL;

	const U32 oldLen = ctx->mapLen;
	U32 newLen = 0;
	if (oldLen == 0) {
		newLen = 256;
	} else {
		newLen = oldLen << 1;
	}

	newKeys = ctx->allocator.alloc(newLen * sizeof(*newKeys));
	assert(newKeys != NULL);
	memZero(newKeys, newLen * sizeof(*newKeys));

	newValues = ctx->allocator.alloc(newLen * sizeof(*newValues));
	assert(newValues != NULL);
	memZero(newValues, newLen * sizeof(*newValues));

	for (Uint mapIndex = 0; mapIndex < newLen; mapIndex++) {
		memZero(&newKeys[mapIndex], sizeof(newKeys[mapIndex]));
		memZero(&newValues[mapIndex], sizeof(newValues[mapIndex]));

		newKeys[mapIndex].fontIndex = npfont_NO_FONT;
		newValues[mapIndex].textureIndex = npfont_NO_TEXTURE;
	}

	for (U32 mapIndex = 0; mapIndex < oldLen; mapIndex++) {
		const npfont_MapKey *key = &oldKeys[mapIndex];
		const Bool isFree = key->fontIndex == npfont_NO_FONT;
		if (!isFree) {
			U32 newIndex = npfont__hashKey(key) % newLen; /* @todo change to & */
			while (newKeys[newIndex].fontIndex != npfont_NO_FONT) {
				newIndex = (newIndex + 1) % newLen; /* @todo change to & */
			}
			newKeys[newIndex] = *key;
			newValues[newIndex] = oldValues[mapIndex];
		}
	}

	ctx->mapLen = newLen;
	ctx->mapKeys = newKeys;
	ctx->mapValues = newValues;

	if (oldKeys != NULL) {
		ctx->allocator.free(oldKeys);
	}
	if (oldValues != NULL) {
		ctx->allocator.free(oldValues);
	}
}

void npfont_init(npfont_Ctx **ctxPtr, Allocator *allocator) {
	Int err = 0;
	*ctxPtr = allocator->alloc(sizeof(npfont_Ctx));
	assert(*ctxPtr != NULL);

	npfont_Ctx *ctx = *ctxPtr;
	memZero(ctx, sizeof(*ctx));

	ctx->allocator = *allocator;
	err = arenaAlloc(&ctx->codepointsArena, KIBI, ctx->allocator);
	assert(!err);
	err = arenaAlloc(&ctx->pixelsArena,     KIBI, ctx->allocator);
	assert(!err);

	/* @todo */
	ctx->mapLen = 0;
	npfont__growMap(ctx);

	//ctx->mapLen = 1024;

	//ctx->mapKeys = allocator->alloc(ctx->mapLen * sizeof(*ctx->mapKeys));
	//memZero(ctx->mapKeys, ctx->mapLen * sizeof(*ctx->mapKeys));

	//ctx->mapValues = allocator->alloc(ctx->mapLen * sizeof(*ctx->mapValues));
	//memZero(ctx->mapValues, ctx->mapLen * sizeof(*ctx->mapValues));

	//for (Uint i = 0; i < ctx->mapLen; i++) {
	//	ctx->mapKeys[i].fontIndex = npfont_NO_FONT;
	//	ctx->mapValues[i].textureIndex = npfont_NO_TEXTURE;
	//}
}

void npfont_deinit(npfont_Ctx *ctx) {
	Usize i = 0;

	ctx->allocator.free(ctx->mapKeys);
	ctx->allocator.free(ctx->mapValues);

	for (i = 0; i < ctx->fontFiles.len; i++) {
		ctx->allocator.free(ctx->fontFiles.items[i]);
	}

	ctx->allocator.free(ctx);
}

/* WTF IS THIS DOING HERE? */
int readWholeFile(const char *filename, U8 **buf, size_t *size) {
	FILE *fp = NULL;
	U8 *internalBuf = NULL;
	int err = 0;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		goto err;
	}

	err = fseek(fp, 0, SEEK_END);
	if (err) {
		goto err;
	}
	size_t ftellSize = ftell(fp);
	if (size != NULL) {
		*size = ftellSize;
	}

	err = fseek(fp, 0, SEEK_SET);
	if (err) {
		goto err;
	}

	if (buf != NULL) {
		internalBuf = malloc(sizeof(U8) * ftellSize);
		if (internalBuf == NULL) {
			goto err;
		}

		size_t freadSize = fread(internalBuf, 1, ftellSize, fp);
		if (freadSize != ftellSize) {
			goto err;
		}

		*buf = internalBuf;
	}

	fclose(fp);

	return 0;
err:
	if (fp != NULL) {
		fclose(fp);
	}
	if (internalBuf != NULL) {
		free(internalBuf);
	}

	return 1;
}

npfont_FontIndex npfont_loadFont(npfont_Ctx *ctx, const char *filepath) {
	U8 *ttfBuffer = NULL;
	Int err = readWholeFile(filepath, &ttfBuffer, NULL);
	if (err) {
		fprintf(stderr, "failed to read file \"%s\"\n", filepath);
		exit(1);
	}

	daAppendAZ(&ctx->fonts, ctx->allocator);

	Int success = stbtt_InitFont(
		&ctx->fonts.items[ctx->fonts.len - 1],
		ttfBuffer,
		stbtt_GetFontOffsetForIndex(ttfBuffer, 0)
	);
	assert(success);
	daAppendA(&ctx->fontFiles, ttfBuffer, ctx->allocator);

	return ctx->fonts.len - 1;
}

void npfont_pushFont(npfont_Ctx *ctx, npfont_FontIndex index, npfont_FontSize size) {
	ctx->fontStack[ctx->fontStackTop].index = index;
	ctx->fontStack[ctx->fontStackTop].size = size;
	ctx->fontStackTop += 1;
	assert(ctx->fontStackTop < FONT_STACK_CAPACITY);
}

void npfont_popFont(npfont_Ctx *ctx, npfont_FontIndex *index, npfont_FontSize *size) {
	assert(ctx->fontStackTop > 0);
	ctx->fontStackTop -= 1;

	if (index != NULL) {
		*index = ctx->fontStack[ctx->fontStackTop].index;
	}

	if (size != NULL) {
		*size = ctx->fontStack[ctx->fontStackTop].size;
	}
}

void npfont_peekFont(npfont_Ctx *ctx, npfont_FontIndex *index, npfont_FontSize *size) {
	assert(ctx->fontStackTop > 0);

	if (index != NULL) {
		*index = ctx->fontStack[ctx->fontStackTop - 1].index;
	}

	if (size != NULL) {
		*size = ctx->fontStack[ctx->fontStackTop - 1].size;
	}
}

npfont_FontSize npfont_sizeFromPoints(
	npfont_Ctx *ctx,
	const npfont_FontIndex fontIndex,
	const I32 points
) {
	const I32 heightInPixels = points * 4 / 3;
	return stbtt_ScaleForPixelHeight(
		&ctx->fonts.items[fontIndex],
		heightInPixels
	);
}

void npfont_setGooeyContext(npfont_Ctx *ctx, gooey_Ctx *gctx) {
	ctx->gctx = gctx;
}

Bool npfont__glyph_eq(const npfont_Glyph *a, const npfont_Glyph *b) {
	if (a->len != b->len) {
		return false;
	}

	for (U32 i = 0; i < a->len; i++) {
		if (a->codepoints[i] != b->codepoints[i]) {
			return false;
		}
	}

	return true;
}

void npfont__mapOccupyIndex(npfont_Ctx *ctx, U32 index, const npfont_Glyph *glyph) {
	Int err = 0;
	const npfont_FontIndex fontIndex = ctx->fontStack[ctx->fontStackTop - 1].index;
	const npfont_FontSize  fontSize  = ctx->fontStack[ctx->fontStackTop - 1].size;
	npfont_MapKey *key = &ctx->mapKeys[index];

	key->fontIndex = fontIndex;
	key->fontSize = fontSize;
	key->glyph.len = glyph->len;

	key->glyph.codepoints = arenaPushArray(&ctx->codepointsArena, U32, glyph->len);
	if (key->glyph.codepoints == NULL) {
		err = arenaAlloc(
			&ctx->codepointsArena,
			ctx->codepointsArena.size << 2,
			ctx->allocator
		);
		assert(!err);

		key->glyph.codepoints = arenaPushArray(&ctx->codepointsArena, U32, glyph->len);
		assert(key->glyph.codepoints != NULL);
	}

	memCopy(key->glyph.codepoints, glyph->codepoints, glyph->len * sizeof(U32));
	ctx->mapCount++;
}

U32 npfont__mapIndexFromGlyph(npfont_Ctx *ctx, npfont_Glyph *glyph, Bool *isFree) {
	if (ctx->mapCount * 2 >= ctx->mapLen) {
		npfont__growMap(ctx);
	}

	Bool keyIsEqual = false;
	const U32 maxIterations = ctx->mapLen - 1;
	const npfont_FontIndex fontIndex = ctx->fontStack[ctx->fontStackTop - 1].index;
	const npfont_FontSize  fontSize  = ctx->fontStack[ctx->fontStackTop - 1].size;
	npfont_MapKey key = {0};
	key.glyph = *glyph;
	key.fontIndex = fontIndex;
	key.fontSize  = fontSize;

	U32 mapIndex = npfont__hashKey(&key) % ctx->mapLen; /* @todo change to & */

	Bool vIsFree = false;

	if (glyph->len != 1) {
		/* @todo deal with multiple codepoints */
		assert(0 && "only len 1 allowed for now");
	}

	for (U32 iterations = 0; iterations < maxIterations; iterations += 1) {
		npfont_MapKey *iterKey = &ctx->mapKeys[mapIndex];

		vIsFree = iterKey->fontIndex == npfont_NO_FONT;
		keyIsEqual = 
				key.fontIndex == iterKey->fontIndex &&
				key.fontSize  == iterKey->fontSize &&
				npfont__glyph_eq(&key.glyph, &iterKey->glyph);

		if (vIsFree || keyIsEqual) {
			if (isFree != NULL) {
				*isFree = vIsFree;
			}

			return mapIndex;
		}

		mapIndex = (mapIndex + 1) % ctx->mapLen; /* @todo change to & */
	}

	assert(0);
	return 1;
}

void npfont_loadGlyphs(npfont_Ctx *ctx, npfont_Glyph *glyphs, U32 glyphsLen) {
	const npfont_FontIndex fontIndex = ctx->fontStack[ctx->fontStackTop - 1].index;
	const npfont_FontSize  fontSize  = ctx->fontStack[ctx->fontStackTop - 1].size;
	Bool isFree = false;

	assert(fontIndex != npfont_NO_FONT);

	stbtt_fontinfo *fontinfo = &ctx->fonts.items[fontIndex];

	for (U32 glyphIndex = 0; glyphIndex < glyphsLen; glyphIndex++) {
		npfont_Glyph *glyph = &glyphs[glyphIndex];

		const U32 mapIndex = npfont__mapIndexFromGlyph(ctx, glyph, &isFree);
		if (!isFree) {
			printf("tried to load glyph that is already loaded\n");
			assert(0);
			continue;
		}
		npfont__mapOccupyIndex(ctx, mapIndex, glyph);

		npfont_MapValue *mval = &ctx->mapValues[mapIndex];

		I32 pixelsWidth  = 0;
		I32 pixelsHeight = 0;
		U32 codepoint = glyph->codepoints[0];
		assert(glyph->len == 1);

		U8 *alphaBitmap = stbtt_GetCodepointBitmap(
			fontinfo,
			0, fontSize,
			codepoint,
			&pixelsWidth,
			&pixelsHeight,
			&mval->xOffset,
			&mval->yOffset
		);

		I32 lsb = 0;
		stbtt_GetCodepointHMetrics(fontinfo, codepoint, &mval->xAdvance, &lsb);
		mval->xAdvance *= fontSize;

		if (alphaBitmap != NULL) {
			daAppendAZ(&ctx->textures, ctx->allocator);
			const U16 textureIndex = ctx->textures.len - 1;
			mval->textureIndex = textureIndex;

			const U32 pixelsSize = pixelsWidth * pixelsHeight * sizeof(U32);
			if (ctx->pixelsArena.size < pixelsSize) {
				assert(
					arenaAlloc(
						&ctx->pixelsArena,
						pixelsSize,
						ctx->allocator
					) != 1
				);
			}

			U32 *pixels = arenaPushArray(
				&ctx->pixelsArena,
				U32,
				pixelsWidth * pixelsHeight
			);
			assert(pixels != NULL);
			for (
				I32 pixelsIndex = 0;
				pixelsIndex < pixelsWidth * pixelsHeight;
				pixelsIndex++
			) {
				pixels[pixelsIndex] = 0xffffff00 | (U32)(alphaBitmap[pixelsIndex]);
			}

			gooey_loadTexture(
				ctx->gctx,
				&ctx->textures.items[textureIndex],
				pixels,
				pixelsWidth,
				pixelsHeight
			);

			arenaReset(&ctx->pixelsArena);

			mval->src = rect_I32(0, 0, pixelsWidth, pixelsHeight);
			mval->width  = pixelsWidth;
			mval->height = pixelsHeight;
		} else {
			fprintf(stderr, "no texture for codepoint %d\n", codepoint);
			mval->textureIndex = npfont_NO_TEXTURE;
		}
	}
}

Int npfont_getFontSize(npfont_Ctx *ctx, I32 fontStackIndex, npfont_FontSize *fontSize) {
	const I32 absoluteFontStackIndex =
		(fontStackIndex < 0) ? ctx->fontStackTop + fontStackIndex : fontStackIndex;
	if (fontSize != NULL) {
		*fontSize = ctx->fontStack[absoluteFontStackIndex].size;
	}
	return 0;
}

Int npfont_getAscent(npfont_Ctx *ctx, I32 fontStackIndex, I32 *ascent) {
	const I32 absoluteFontStackIndex =
		(fontStackIndex < 0) ? ctx->fontStackTop + fontStackIndex : fontStackIndex;
	const I32 fontIndex = ctx->fontStack[absoluteFontStackIndex].index;
	stbtt_GetFontVMetrics(&ctx->fonts.items[fontIndex], ascent, NULL, NULL);
	return 0;
}

Int npfont_getDescent(npfont_Ctx *ctx, I32 fontStackIndex, I32 *descent) {
	const I32 absoluteFontStackIndex =
		(fontStackIndex < 0) ? ctx->fontStackTop + fontStackIndex : fontStackIndex;
	const I32 fontIndex = ctx->fontStack[absoluteFontStackIndex].index;
	stbtt_GetFontVMetrics(&ctx->fonts.items[fontIndex], NULL, descent, NULL);
	return 0;
}

Int npfont_getLineGap(npfont_Ctx *ctx, I32 fontStackIndex, I32 *lineGap) {
	const I32 absoluteFontStackIndex =
		(fontStackIndex < 0) ? ctx->fontStackTop + fontStackIndex : fontStackIndex;
	const I32 fontIndex = ctx->fontStack[absoluteFontStackIndex].index;
	stbtt_GetFontVMetrics(&ctx->fonts.items[fontIndex], NULL, NULL, lineGap);
	return 0;
}

Int npfont_getGlyphInfo(
	npfont_Ctx *ctx,
	U32 *codepoints,
	U32 codepointsLen,
	npfont_GlyphInfo *glyphInfo
) {
	npfont_Glyph glyph = {0};
	U32 mapIndex = 0;
	npfont_MapKey *mkey = NULL;
	npfont_MapValue *mval = NULL;
	Bool isFree = false;

	glyph.len = codepointsLen;
	glyph.codepoints = codepoints;
	mapIndex = npfont__mapIndexFromGlyph(ctx, &glyph, &isFree);
	assert(!isFree);

	mkey = &ctx->mapKeys[mapIndex];
	mval = &ctx->mapValues[mapIndex];

	glyphInfo->fontSize = mkey->fontSize;
	glyphInfo->xAdvance = mval->xAdvance;
	glyphInfo->xOffset = mval->xOffset;
	glyphInfo->yOffset = mval->yOffset;
	glyphInfo->textureIndex = mval->textureIndex;
	glyphInfo->src = mval->src;
	glyphInfo->dst.x = mval->xOffset;
	glyphInfo->dst.y = mval->yOffset;
	glyphInfo->dst.w = mval->width;
	glyphInfo->dst.h = mval->height;

	stbtt_GetFontVMetrics(
		&ctx->fonts.items[mkey->fontIndex],
		&glyphInfo->ascent,
		&glyphInfo->descent,
		&glyphInfo->lineGap
	);

	return 0;
}

#endif /* defined(NPFONT_IMPLEMENTATION) */
