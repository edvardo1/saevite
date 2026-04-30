#if !defined(ACYACSL_DOT_H)
#define ACYACSL_DOT_H

#define TODO(msg) \
	( \
		fprintf( \
			stderr, \
			"\x1b[41m[TODO]\x1b[0m at %s:%d \x1b[35m\"%s\"\x1b[0m\n", \
			__FILE__, __LINE__, msg \
		), \
		fflush(stderr), \
		exit(1) \
	)
#define UNUSED(x) ((Void)(x))

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(n, lo, hi) MIN(MAX((n), (lo)), (hi))
#define CLAMPTOP(a, b) MIN(a, b)
#define CLAMPBOT(a, b) MAX(a, b)
#define IN_BOUNDS(n, lo, hi) ((lo <= n) && (n <= hi))
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define SIGN(a) ((a) == 0 ? 0 : ((a) < 0 : -1 : 1))
#define MAXABS(a, b) (ABS(a) > ABS(b) ? (a) : (b))
#define MINABS(a, b) (ABS(a) < ABS(b) ? (a) : (b))

#define S8(cstr) (string8FromCstr(cstr, sizeof(cstr) - 1))
#define S S8
#define S8lenbuf(str) (Int)(str).len, (str).buf
#define Slens S8lenbuf
#define string8 string8FromCstr
#define arenaPushArray(arena, type, len) \
	arenaPush((arena), sizeof(type) * (len), sizeof(type))
#define arenaPushType(arena, type) \
	arenaPush((arena), sizeof(type), sizeof(Usize))
#define arenaPushStruct(arena, type) \
	arenaPush((arena), sizeof(type), sizeof(Usize))

/* @todo use arenaAlloc everywhere instead */
#define arenaMalloc arenaAlloc

#define PICO (10e-12)
#define NANO (10e-9)
#define MICRO (10e-6)
#define MILLI (10e-3)
#define KILO (10e3)
#define MEGA (10e6)
#define GIGA (10e9)
#define TERA (10e12)

#define KIBI (1024)
#define MIBI (KIBI * KIBI)
#define GIBI (KIBI * KIBI * KIBI)

#define HUNDRED (100)
#define THOUSAND (1000)
#define MILLION (THOUSAND * THOUSAND)
#define BILLION (THOUSAND * MILLION)
#define TRILLION (THOUSAND * BILLION)

#if defined(ACYACSL_USE_STDINT)
typedef int64_t I64;
typedef int32_t I32;
typedef int16_t I16;
typedef int8_t I8;
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
#else
typedef signed long int I64;
typedef signed int I32;
typedef signed short I16;
typedef signed char I8;
typedef unsigned long int U64;
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;
#endif

#if __STDC_VERSION__ >= 199901L /* C99 or later */
typedef _Bool Bool;
#else
#if !defined(Bool)
typedef U8 Bool;
#endif /* !defined(Bool) */
#if !defined(false)
#define false ((Bool)0)
#endif /* !defined(false) */
#if !defined(true)
#define true ((Bool)1)
#endif /* !defined(true) */
#endif /* __STDC_VERSION__ >= 199901L */

typedef float F32;
typedef double F64;
typedef U8 B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef size_t Usize;
typedef float Float;
typedef double Double;
typedef unsigned long Ulong;
typedef long Long;
typedef unsigned int Uint;
typedef int Int;
typedef unsigned short Ushort;
typedef short Short;
typedef unsigned char Uchar;
typedef char Char;
typedef unsigned char Byte;
typedef void Void;
typedef Void *RawPtr;

typedef struct Allocator {
	Void *(*alloc)(Usize);
	Void (*free)(Void *);
} Allocator;

typedef struct String8 {
	Usize len;
	Char *buf;
} String8;

typedef struct Arena {
	Byte *memory;
	Usize size;
	Usize point;
} Arena;

#define CONCAT2__(a, b) a##b
#define CONCAT2_(a, b) CONCAT2__(a, b)
#define CONCAT2(a, b) CONCAT2_(a, b)

/* @todo deprecate genDa */
#define genDa(type) \
	struct { \
		Usize len; \
		Usize capacity; \
		type *items; \
	}
#define DynamicArray genDa

#define Slice(type) \
	struct { \
		Usize len; \
		type *items; \
	}

#define daAt_(da, index) \
	((index < da->len) \
	 ? (da->items[index]) \
	 : (printError(S("out of bounds")), da->items[index]))
#define daAt(da, index) \
	daAt_((da), (index))

#define daAtPut_(da, index, value) \
	((index < da->len) \
	 ? (da->items[index] = value) \
	 : (printError(S("out of bounds")), da->items[index] = value))
#define daAtPut(da, index, value) \
	daAtPut_((da), (index), (value))

#define daAtFirst(da) \
	daAt((da), 0)

#define daAtFirstPut(da, value) \
	daAtPut((da), 0, (value))

#define daAtLast(da) \
	daAt((da), (da)->len - 1)

#define daAtLastPut(da, value) \
	daAtPut((da), (da)->len - 1, (value))

Usize daAppendAEHelper(
	Void **items,
	Allocator allocator,
	Usize size,
	Usize copySize
);

#define daAppendAE_(da, value, allocator, err) \
	do { \
		if (da->len + 1 >= da->capacity) { \
			if (da->capacity == 0) { \
				da->capacity = 16; \
			} else { \
				da->capacity <<= 2; \
			} \
			*err = daAppendAEHelper( \
				(Void **)&da->items, \
				allocator, \
				da->capacity * sizeof(*da->items), \
				da->len * sizeof(*da->items) \
			); \
		} else { \
			*err = 0; \
		} \
		\
		if (!*err) { \
			da->items[da->len] = value; \
			da->len += 1; \
		} \
	} while(0)
#define daAppendAE(da, value, allocator, err) \
	daAppendAE_((da), (value), (allocator), (err))

#define daAppendAEZ_(da, allocator, err) \
	do { \
		if (da->len + 1 >= da->capacity) { \
			if (da->capacity == 0) { \
				da->capacity = 16; \
			} else { \
				da->capacity <<= 2; \
			} \
			*err = daAppendAEHelper( \
				(Void **)&da->items, \
				allocator, \
				da->capacity * sizeof(*da->items), \
				da->len * sizeof(*da->items) \
			); \
		} else { \
			*err = 0; \
		} \
		\
		if (!*err) { \
			memZero(&da->items[da->len], sizeof(*da->items)); \
			da->len += 1; \
		} \
	} while(0)
#define daAppendAEZ(da, allocator, err) \
	daAppendAEZ_((da), (allocator), (err))


#define daAppendA_(da, value, allocator) \
	do { \
		if (da->len + 1 >= da->capacity) { \
			if (da->capacity == 0) { \
				da->capacity = 16; \
			} else { \
				da->capacity <<= 2; \
			} \
			if (daAppendAEHelper( \
				(Void **)&da->items, \
				allocator, \
				da->capacity * sizeof(*da->items), \
				da->len * sizeof(*da->items) \
			)) { \
				printError(S("error during daAppendA_")); \
			} \
		} \
		\
		da->items[da->len] = value; \
		da->len += 1; \
	} while(0)
#define daAppendA(da, value, allocator) daAppendA_((da), (value), (allocator))

#define daAppend(da, value) daAppendA((da), (value), (stdAllocator))

#define daAppendAZ_(da, allocator) \
	do { \
		if (da->len + 1 >= da->capacity) { \
			if (da->capacity == 0) { \
				da->capacity = 16; \
			} else { \
				da->capacity <<= 2; \
			} \
			if (daAppendAEHelper( \
				(Void **)&da->items, \
				allocator, \
				da->capacity * sizeof(*da->items), \
				da->len * sizeof(*da->items) \
			)) { \
				printError(S("error during daAppendA_")); \
			} \
		} \
		\
		memZero(&da->items[da->len], sizeof(*da->items)); \
		da->len += 1; \
	} while(0)
#define daAppendAZ(da, allocator) daAppendAZ_((da), (allocator))

#define daAppendZ(da) daAppendAZ((da), (stdAllocator))

#define daFree(da, allocator) \
	do { \
		if ((da)->items != NULL) { \
			(allocator).free((da)->items); \
			(da)->items = NULL; \
		} \
	} while(0)


#define VEC2NAME(type) CONCAT2(Vec2_, type)
#define DECLAREVEC2(type, name, prefix) \
	typedef struct name { \
		type x, y; \
	} name; \
	\
	type *CONCAT2(prefix, _asPtr)(name *a); \
	name prefix (type x, type y); \
	name CONCAT2(prefix, _add)(name a, name b); \
	name CONCAT2(prefix, _sub)(name a, name b); \
	name CONCAT2(prefix, _mul)(name a, name b); \
	name CONCAT2(prefix, _div)(name a, name b);

#define DEFINEVEC2(type, name, prefix) \
	type *CONCAT2(prefix, _asPtr)(name *a) { \
		return (type *)(a); \
	} \
	name prefix (type x, type y) { \
		name a; \
		a.x = x; \
		a.y = y; \
		return a; \
	} \
	name CONCAT2(prefix, _add)(name a, name b) { \
		name c; \
		c.x = a.x + b.x; \
		c.y = a.y + b.y; \
		return c; \
	} \
	name CONCAT2(prefix, _sub)(name a, name b) { \
		name c; \
		c.x = a.x - b.x; \
		c.y = a.y - b.y; \
		return c; \
	} \
	name CONCAT2(prefix, _mul)(name a, name b) { \
		name c; \
		c.x = a.x * b.x; \
		c.y = a.y * b.y; \
		return c; \
	} \
	name CONCAT2(prefix, _div)(name a, name b) { \
		name c; \
		c.x = a.x / b.x; \
		c.y = a.y / b.y; \
		return c; \
	} \

DECLAREVEC2(U8,  Vec2_U8,  vec2_U8)
DECLAREVEC2(U16, Vec2_U16, vec2_U16)
DECLAREVEC2(U32, Vec2_U32, vec2_U32)
DECLAREVEC2(U64, Vec2_U64, vec2_U64)
DECLAREVEC2(I8,  Vec2_I8,  vec2_I8)
DECLAREVEC2(I16, Vec2_I16, vec2_I16)
DECLAREVEC2(I32, Vec2_I32, vec2_I32)
DECLAREVEC2(I64, Vec2_I64, vec2_I64)
DECLAREVEC2(F32, Vec2_F32, vec2_F32)
DECLAREVEC2(F64, Vec2_F64, vec2_F64)

#define RECTNAME(type) CONCAT2(Rect_, type)

#define DECLARERECT(type, name, prefix) \
	typedef struct name { \
		type x, y, w, h; \
	} name; \
	\
	type *CONCAT2(prefix, _asPtr)(name *a); \
	name prefix(type x, type y, type w, type h); \
	name CONCAT2(make_, name)(type x, type y, type w, type h); \
	name CONCAT2(prefix, _fromVecs)(CONCAT2(Vec2_, type) a, CONCAT2(Vec2_, type) b);

#define DEFINERECT(type, name, prefix) \
	type *CONCAT2(prefix, _asPtr)(name *a) { \
		return (type *)(a); \
	} \
	name prefix(type x, type y, type w, type h) { \
		name a; \
		a.x = x; \
		a.y = y; \
		a.w = w; \
		a.h = h; \
		return a; \
	} \
	name CONCAT2(make_, name)(type x, type y, type w, type h) { \
		name a; \
		a.x = x; \
		a.y = y; \
		a.w = w; \
		a.h = h; \
		return a; \
	} \
	name CONCAT2(prefix, _fromVecs)(CONCAT2(Vec2_, type) a, CONCAT2(Vec2_, type) b) { \
		name r; \
		r.x = a.x; \
		r.y = a.y; \
		r.w = b.x; \
		r.h = b.y; \
		return r; \
	}

#define RECT_LEFT(r) ((r).x)
#define RECT_RIGHT(r) ((r).x + (r).w)
#define RECT_TOP(r) ((r).y)
#define RECT_BOTTOM(r) ((r).y + (r).h)
#define RECT_UP RECT_TOP
#define RECT_DOWN RECT_BOTTOM

DECLARERECT(U8,  Rect_U8,  rect_U8)
DECLARERECT(U16, Rect_U16, rect_U16)
DECLARERECT(U32, Rect_U32, rect_U32)
DECLARERECT(U64, Rect_U64, rect_U64)
DECLARERECT(I8,  Rect_I8,  rect_I8)
DECLARERECT(I16, Rect_I16, rect_I16)
DECLARERECT(I32, Rect_I32, rect_I32)
DECLARERECT(I64, Rect_I64, rect_I64)
DECLARERECT(F32, Rect_F32, rect_F32)
DECLARERECT(F64, Rect_F64, rect_F64)

String8 string8FromCstr(char *cstr, Usize len);
Bool memEq(const Void *a, const Void *b, Usize len);
Bool strEq(const String8 a, const String8 b);
String8 strSlice(String8 str, const Usize idx, const Usize len);
Void memZero(Void *ptr, Usize len);
Void memCopy(Void *dst, Void *src, Usize size);
Int arenaAlloc(Arena *arena, Usize size, Allocator allocator);
Void arenaFree(Arena *arena, Allocator allocator);
Void arenaZero(Arena *arena);
Void arenaReset(Arena *arena);
Void *arenaPush(Arena *arena, Usize size, Usize align);
Ulong djb2(const Void *ptr, Usize len);
Void printError(String8 str);

extern Allocator stdAllocator;

#endif /* !defined(ACYACSL_DOT_H) */

#if defined(ACYACSL_IMPLEMENTATION)

Usize daAppendAEHelper(
	Void **items,
	Allocator allocator,
	Usize size,
	Usize copySize
) {
	Void *newPtr = allocator.alloc(size);
	Void *oldPtr = *items;

	if (newPtr == NULL) {
		return 1;
	}

	if (oldPtr != NULL) {
		memCopy(newPtr, oldPtr, copySize);
		allocator.free(oldPtr);
	}

	*items = newPtr;
	return 0;
}

String8 string8FromCstr(char *cstr, Usize len) {
	String8 str8 = {0};
	str8.len = len;
	str8.buf = cstr;
	return str8;
}

Bool memEq(const Void *a, const Void *b, Usize len) {
	Usize i = 0;
	const Byte *ba = (const Byte *)a;
	const Byte *bb = (const Byte *)b;
	for (; i < len; i++) {
		if (ba[i] != bb[i]) {
			return 0;
		}
	}
	return 1;
}

Bool strEq(const String8 a, const String8 b) {
	if (a.len == b.len) {
		return memEq(a.buf, b.buf, a.len);
	}
	return 0;
}

String8 strSlice(String8 str, const Usize idx, const Usize len) {
	String8 out = {0};
	out.buf = &str.buf[idx];
	out.len = len;
	return out;
}

Void memZero(Void *ptr, Usize len) {
	memset(ptr, 0, len);
}

Void memCopy(Void *dst, Void *src, Usize size) {
	Byte *bdst = (Byte *)dst;
	Byte *bsrc = (Byte *)src;
	Usize i = 0;
	for (; i < size; i++) {
		bdst[i] = bsrc[i];
	}
}

/* @todo make allocator a pointer */
Int arenaAlloc(Arena *arena, Usize size, Allocator allocator) {
	if (arena->memory != NULL) {
		arenaFree(arena, allocator);
	}

	arena->memory = (Byte *)allocator.alloc(size);
	if (arena->memory == NULL) {
		return 1;
	}
	/* memZero(arena->memory, size); */
	arena->point = 0;
	arena->size = size;

	return 0;
}

Void arenaFree(Arena *arena, Allocator allocator) {
	if (arena->memory != NULL) {
		allocator.free(arena->memory);
		arena->memory = NULL;
	}
}

Void arenaZero(Arena *arena) {
	Usize i = 0;
	for (; i < arena->size; i++) {
		arena->memory[i] = 0;
	}
}

Void arenaReset(Arena *arena) {
	arena->point = 0;
}

Void *arenaPush(Arena *arena, Usize size, Usize align) {
	Void *out = NULL;
	const Usize padding = (-arena->point) & (align - 1);
	if (arena->point + size + padding > arena->size) {
		return NULL;
	} else {
		out = &arena->memory[arena->point + padding];
		arena->point += size + padding;
		return out;
	}
}

Ulong djb2(const Void *ptr, Usize len) {
	const Byte *buf = (const Byte *)ptr;
	Ulong ret = 5381;
	Usize i = 0;
	for (; i < len; i++) {
		ret = ((ret << 5) + ret) + buf[i];
	}
	return ret;
}

Void printError(String8 str) {
	fprintf(stderr, "ERROR: \"%*s\"\n", (int)str.len, str.buf);
	fprintf(stderr, "segfaulting now...\n");
	fflush(stderr);
	assert(0);
}

DEFINEVEC2(U8,  Vec2_U8,  vec2_U8)
DEFINEVEC2(U16, Vec2_U16, vec2_U16)
DEFINEVEC2(U32, Vec2_U32, vec2_U32)
DEFINEVEC2(U64, Vec2_U64, vec2_U64)
DEFINEVEC2(I8,  Vec2_I8,  vec2_I8)
DEFINEVEC2(I16, Vec2_I16, vec2_I16)
DEFINEVEC2(I32, Vec2_I32, vec2_I32)
DEFINEVEC2(I64, Vec2_I64, vec2_I64)
DEFINEVEC2(F32, Vec2_F32, vec2_F32)
DEFINEVEC2(F64, Vec2_F64, vec2_F64)

DEFINERECT(U8,  Rect_U8,  rect_U8)
DEFINERECT(U16, Rect_U16, rect_U16)
DEFINERECT(U32, Rect_U32, rect_U32)
DEFINERECT(U64, Rect_U64, rect_U64)
DEFINERECT(I8,  Rect_I8,  rect_I8)
DEFINERECT(I16, Rect_I16, rect_I16)
DEFINERECT(I32, Rect_I32, rect_I32)
DEFINERECT(I64, Rect_I64, rect_I64)
DEFINERECT(F32, Rect_F32, rect_F32)
DEFINERECT(F64, Rect_F64, rect_F64)

#endif /* defined(ACYACSL_IMPLEMENTATION) */
