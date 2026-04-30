#if !defined(NPUNICODE_H__)
#define NPUNICODE_H__

typedef struct npunicode_Utf8ArrayDecoder npunicode_Utf8ArrayDecoder;
typedef struct npunicode_Utf8IteratorDecoder npunicode_Utf8IteratorDecoder;
typedef struct npunicode_Utf8IteratorDecoder_FunctionReturn npunicode_Utf8IteratorDecoder_FunctionReturn;
typedef
	npunicode_Utf8IteratorDecoder_FunctionReturn
	(*npunicode_Utf8IteratorDecoder_Function)(Void *data);

struct npunicode_Utf8ArrayDecoder {
	String8 string;
	Usize index;
};

struct npunicode_Utf8IteratorDecoder_FunctionReturn {
	U8 byte;
	B8 ok;
};

struct npunicode_Utf8IteratorDecoder {
	npunicode_Utf8IteratorDecoder_Function next;
	Void *data;
};

#define npunicode_SUCCESS 0
#define npunicode_DONE    1
#define npunicode_INVALID 2

npunicode_Utf8ArrayDecoder npunicode_utf8ArrayDecoder(String8 str, Usize index);

Int npunicode_utf8ArrayDecoder_popCodepoint(npunicode_Utf8ArrayDecoder *decoder, U32 *codepoint);
Int npunicode_utf8ArrayDecoder_peekCodepoint(npunicode_Utf8ArrayDecoder *decoder, U32 *codepoint);
Int npunicode_utf8ArrayDecoder_nextCodepoint(npunicode_Utf8ArrayDecoder *decoder);
Int npunicode_utf8ArrayDecoder_prevCodepoint(npunicode_Utf8ArrayDecoder *decoder);

npunicode_Utf8IteratorDecoder npunicode_utf8IteratorDecoder(npunicode_Utf8IteratorDecoder_Function next, Void *data);
Int npunicode_utf8IteratorDecoder_popCodepoint(npunicode_Utf8IteratorDecoder *decoder, U32 *codepoint);

#if defined(NPUNICODE_IMPLEMENTATION)

npunicode_Utf8ArrayDecoder npunicode_utf8ArrayDecoder(String8 string, Usize index) {
	npunicode_Utf8ArrayDecoder decoder = {0};
	decoder.string = string;
	decoder.index = index;
	return decoder;
}

Int npunicode_utf8ArrayDecoder_popCodepoint(npunicode_Utf8ArrayDecoder *decoder, U32 *codepoint) {
	U8 bytes[4] = {0};
	U32 tmpCodepoint = 0;

	if (decoder->index >= decoder->string.len) {
		return npunicode_DONE;
	}
	bytes[0] = (U8)decoder->string.buf[decoder->index];

	if ((bytes[0] & 0x80) == 0) { /* 1 byte */
		tmpCodepoint = bytes[0];
		*codepoint = tmpCodepoint;

		decoder->index += 1;
		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xe0) == 0xc0) { /* 2 bytes */
		if (decoder->index + 1 >= decoder->string.len) {
			return npunicode_INVALID;
		}

		bytes[1] = (U8)decoder->string.buf[decoder->index + 1];

		if ((bytes[1] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x1f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		*codepoint = tmpCodepoint;

		decoder->index += 2;
		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xf0) == 0xe0) { /* 3 bytes */
		if (decoder->index + 2 >= decoder->string.len) {
			return npunicode_INVALID;
		}

		bytes[1] = (U8)decoder->string.buf[decoder->index + 1];
		bytes[2] = (U8)decoder->string.buf[decoder->index + 2];

		if ((bytes[1] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}
		if ((bytes[2] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x0f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[2] & 0x3f;
		*codepoint = tmpCodepoint;

		decoder->index += 3;
		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xf8) == 0xf0) { /* 4 bytes */
		if (decoder->index + 3 >= decoder->string.len) {
			return npunicode_INVALID;
		}

		bytes[1] = (U8)decoder->string.buf[decoder->index + 1];
		bytes[2] = (U8)decoder->string.buf[decoder->index + 2];
		bytes[3] = (U8)decoder->string.buf[decoder->index + 3];

		if ((bytes[1] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}
		if ((bytes[2] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}
		if ((bytes[3] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x03;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[2] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[3] & 0x3f;
		*codepoint = tmpCodepoint;

		decoder->index += 4;
		return npunicode_SUCCESS;
	} else {
		return npunicode_INVALID;
	}
}

Int npunicode_utf8ArrayDecoder_peekCodepoint(npunicode_Utf8ArrayDecoder *decoder, U32 *codepoint) {
	Usize prevIndex = decoder->index;
	Int code = npunicode_utf8ArrayDecoder_popCodepoint(decoder, codepoint);
	decoder->index = prevIndex;
	return code;
}

Int npunicode_utf8ArrayDecoder_nextCodepoint(npunicode_Utf8ArrayDecoder *decoder) {
	UNUSED(decoder);
	TODO("npunicode_utf8ArrayDecoder_nextCodepoint is unimplemented\n");
	return 0;
}

Int npunicode_utf8ArrayDecoder_prevCodepoint(npunicode_Utf8ArrayDecoder *decoder) {
	UNUSED(decoder);
	TODO("npunicode_utf8ArrayDecoder_prevCodepoint is unimplemented\n");
	return 0;
}

npunicode_Utf8IteratorDecoder npunicode_utf8IteratorDecoder(
	npunicode_Utf8IteratorDecoder_Function next,
	Void *data
) {
	npunicode_Utf8IteratorDecoder decoder = {0};
	decoder.next = next;
	decoder.data = data;
	return decoder;
}

static Bool npunicode__utf8IteratorDecoder_getByte(
	npunicode_Utf8IteratorDecoder *decoder,
	Byte *byte
) {
	npunicode_Utf8IteratorDecoder_FunctionReturn result = {0};
	result = decoder->next(decoder->data);
	if (result.ok) {
		*byte = result.byte;
		return true;
	} else {
		return false;
	}
}

Int npunicode_utf8IteratorDecoder_popCodepoint(
	npunicode_Utf8IteratorDecoder *decoder,
	U32 *codepoint
) {
	U8 bytes[4] = {0};
	U32 tmpCodepoint = 0;

	if (!npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[0])) {
		return npunicode_DONE;
	}

	if ((bytes[0] & 0x80) == 0) { /* 1 byte */
		tmpCodepoint = bytes[0];
		*codepoint = tmpCodepoint;

		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xe0) == 0xc0) { /* 2 bytes */
		if (!npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[1])) {
			return npunicode_INVALID;
		}

		if ((bytes[1] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x1f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		*codepoint = tmpCodepoint;

		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xf0) == 0xe0) { /* 3 bytes */
		if (!npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[1]) ||
		    !npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[2])) {
			return npunicode_INVALID;
		}

		if ((bytes[1] & 0xc0) != 0x80 || (bytes[2] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x0f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[2] & 0x3f;
		*codepoint = tmpCodepoint;

		return npunicode_SUCCESS;
	} else if ((bytes[0] & 0xf8) == 0xf0) { /* 4 bytes */
		if (!npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[1]) ||
		    !npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[2]) ||
		    !npunicode__utf8IteratorDecoder_getByte(decoder, &bytes[3])) {
			return npunicode_INVALID;
		}

		if ((bytes[1] & 0xc0) != 0x80 ||
		    (bytes[2] & 0xc0) != 0x80 ||
		    (bytes[3] & 0xc0) != 0x80) {
			return npunicode_INVALID;
		}

		tmpCodepoint = bytes[0] & 0x03;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[1] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[2] & 0x3f;
		tmpCodepoint <<= 6;
		tmpCodepoint |= bytes[3] & 0x3f;
		*codepoint = tmpCodepoint;

		return npunicode_SUCCESS;
	} else {
		return npunicode_INVALID;
	}
}

#endif /* defined(NPUNICODE_IMPLEMENTATION) */

#endif /* !defined(NPUNICODE_H__) */
