#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ste_text.h"

Void printPieceString(String8 string) {
	Usize i = 0;
	Char c = 0;
	printf("%ld:«", string.len);
	for (i = 0; i < string.len; i++) {
		c = string.buf[i];
		switch (c) {
			case '\r':
				printf("\\r");
				break;
			case '\n':
				printf("\\n");
				break;
			case '\t':
				printf("\\t");
				break;
			default:
				fputc(c, stdout);
				break;
		}
	}
	printf("»");
}

Void ste_printBuffer(ste_Buffer *buffer) {
	Usize i = 0;
	printf("all pieces:\n");
	for (i = 0; i < buffer->allPieces.len; i++) {
		printf("  [%ld] = ", i);
		printPieceString(buffer->allPieces.items[i]);
		printf("\n");
	}
	printf("current pieces:\n");
	for (i = 0; i < buffer->currentPieces.len; i++) {
		printf("  [%ld] = %d ", i, buffer->currentPieces.items[i]);
		printPieceString(buffer->allPieces.items[buffer->currentPieces.items[i]]);
		printf("\n");
	}
	printf("\n");
	printf("\n");
}

Void ste_stringFromBuffer(ste_Buffer *buffer, String8 *string) {
	Uint currentPiecesIndex = 0;
	Uint pieceIndex = 0;
	Uint totalLength = 0;
	String8 pieceString = {0};

	for (
		currentPiecesIndex = 0;
		currentPiecesIndex < buffer->currentPieces.len;
		currentPiecesIndex++
	) {
		pieceIndex = buffer->currentPieces.items[currentPiecesIndex];
		totalLength += buffer->allPieces.items[pieceIndex].len;
	}

	string->len = 0;
	string->buf = malloc(totalLength);
	assert(string->buf != NULL);

	for (
		currentPiecesIndex = 0;
		currentPiecesIndex < buffer->currentPieces.len;
		currentPiecesIndex++
	) {
		pieceIndex = buffer->currentPieces.items[currentPiecesIndex];
		pieceString = buffer->allPieces.items[pieceIndex];
		memcpy(&string->buf[string->len], pieceString.buf, pieceString.len);
		string->len += pieceString.len;
	}
	assert(string->len == totalLength);
}

Void ste_printBufferContents(ste_Buffer *buffer) {
	Usize i = 0;
	for (i = 0; i < buffer->currentPieces.len; i++) {
		printf(
			"%.*s",
			Slens(buffer->allPieces.items[buffer->currentPieces.items[i]])
		);
	}
}

Int ste__buffer_getPieceInfoFromPosition(ste_Buffer *buffer, Uint position, Uint *pieceIndex, Uint *len) {
	Uint index = 0;
	String8 str = {0};
	Uint cumPosition = position;

	for (index = 0; index < buffer->currentPieces.len; index++) {
		str = buffer->allPieces.items[buffer->currentPieces.items[index]];

		if (cumPosition < str.len) {
			if (pieceIndex != NULL) {
				*pieceIndex = index;
			}
			if (len != NULL) {
				*len = cumPosition;
			}
			return 0;
		} else {
			cumPosition -= str.len;
		}
	}

	return 1;
}

Void ste_pieceNew(ste_Buffer *buffer, String8 str, Uint *index) {
	daAppend(&buffer->allPieces, str);
	if (index != NULL) {
		*index = buffer->allPieces.len - 1;
	}
}

Void ste_pieceInsert(ste_Buffer *buffer, Uint currentPiecesPosition, Uint allPiecesIndex) {
	Usize i = 0;
	assert(allPiecesIndex < buffer->allPieces.len);
	daAppendZ(&buffer->currentPieces);

	memmove(
		&buffer->currentPieces.items[currentPiecesPosition + 1],
		&buffer->currentPieces.items[currentPiecesPosition],
		(buffer->currentPieces.len - 1 - currentPiecesPosition) *
		sizeof(*buffer->currentPieces.items)
	);
	buffer->currentPieces.items[currentPiecesPosition] = allPiecesIndex;

	daAppend(
		&buffer->actions,
		ste_makeInsertAction(
			currentPiecesPosition,
			allPiecesIndex
		)
	);
}

Void ste_newPieceInsert(ste_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	ste_pieceNew(buffer, string, &pieceIndex);
	ste_pieceInsert(buffer, currentPiecesPosition, pieceIndex);
}

Void ste__buffer_pieceGetString(ste_Buffer *buffer, Uint currentPiecesPosition, String8 *str) {
	*str = buffer->allPieces.items[
		buffer->currentPieces.items[currentPiecesPosition]
	];
}

Void ste__buffer_pieceReplace(
	ste_Buffer *buffer,
	Uint currentPiecesPosition,
	Uint allPiecesIndex
) {
	daAppend(
		&buffer->actions,
		ste_makeReplaceAction(
			currentPiecesPosition,
			buffer->currentPieces.items[currentPiecesPosition],
			allPiecesIndex
		)
	);
	buffer->currentPieces.items[currentPiecesPosition] = allPiecesIndex;
}

Void ste__buffer_newPieceReplace(ste_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	ste_pieceNew(buffer, string, &pieceIndex);
	ste__buffer_pieceReplace(buffer, currentPiecesPosition, pieceIndex);
}

Void ste__buffer_pieceRemove(ste_Buffer *buffer, Uint currentPiecesPosition) {
	memmove(
		&buffer->currentPieces.items[currentPiecesPosition],
		&buffer->currentPieces.items[currentPiecesPosition + 1],
		(buffer->currentPieces.len - 1 - currentPiecesPosition) *
		sizeof(*buffer->currentPieces.items)
	);
	buffer->currentPieces.len -= 1;
	daAppend(
		&buffer->actions,
		ste_makeRemoveAction(currentPiecesPosition, currentPiecesPosition)
	);
}

Void ste_insertString(ste_Buffer *buffer, Uint position, String8 str) {
	String8 oldStr = {0};
	Uint index = 0;
	Uint pieceIndex = 0, len = 0;
	Uint allPiecesIndex = 0;

	buffer->mode = ste_BufferMode_None;

	ste__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
	allPiecesIndex = buffer->currentPieces.items[pieceIndex];
	ste__buffer_pieceGetString(buffer, pieceIndex, &oldStr);

	ste__buffer_pieceRemove(buffer, pieceIndex);
	if (len == 0) {
		ste_pieceInsert(buffer, pieceIndex, allPiecesIndex);
	} else if (oldStr.len - len > 0) {
		ste_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, len, oldStr.len - len));
	}

	ste_newPieceInsert(buffer, pieceIndex, str);

	if (len == oldStr.len) {
		ste_pieceInsert(buffer, pieceIndex, allPiecesIndex);
	} else if (len > 0) {
		ste_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
	}
}

Void ste_insertChar(ste_Buffer *buffer, Uint position, Char c) {
	String8 oldStr = {0};
	String8 str = {0};
	Uint index = 0;
	Uint pieceIndex = 0, len = 0;
	Uint cpIndex = 0;
	Uint allPiecesIndex = 0;
	Int err = 0;

	if (
		buffer->mode == ste_BufferMode_InsertingChars &&
		position == buffer->lastPosition + 1
	) {
		buffer->allPieces.items[buffer->lastCharAllPiecesIndex].buf = 
			realloc(
				buffer->allPieces.items[buffer->lastCharAllPiecesIndex].buf,
				buffer->allPieces.items[buffer->lastCharAllPiecesIndex].len + 1
			);
		buffer->allPieces.items[buffer->lastCharAllPiecesIndex].len += 1;
		buffer->allPieces.items[buffer->lastCharAllPiecesIndex].buf[
			buffer->allPieces.items[buffer->lastCharAllPiecesIndex].len - 1
		] = c;
		buffer->lastPosition = position;
	} else {
		str.buf = malloc(1);
		assert(str.buf != NULL);
		str.buf[0] = c;
		str.len = 1;
		ste_pieceNew(buffer, str, &cpIndex);

		buffer->mode = ste_BufferMode_InsertingChars;
		buffer->lastPosition = position;

		if (buffer->currentPieces.len > 0) {
			err = ste__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		}

		if (buffer->currentPieces.len <= 0 || err) {
			if (position == 0) {
				ste_pieceInsert(buffer, 0, cpIndex);
				buffer->lastCharAllPiecesIndex = buffer->currentPieces.items[0];
			} else {
				ste_pieceInsert(buffer, buffer->currentPieces.len, cpIndex);
				buffer->lastCharAllPiecesIndex = buffer->allPieces.len - 1;
			}
		} else {
			allPiecesIndex = buffer->currentPieces.items[pieceIndex];
			ste__buffer_pieceGetString(buffer, pieceIndex, &oldStr);

			ste__buffer_pieceRemove(buffer, pieceIndex);
			if (len == 0) {
				ste_pieceInsert(buffer, pieceIndex, allPiecesIndex);
			} else if (oldStr.len - len > 0) {
				ste_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, len, oldStr.len - len));
			}

			ste_pieceInsert(buffer, pieceIndex, cpIndex);
			buffer->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];

			if (len == oldStr.len) {
				ste_pieceInsert(buffer, pieceIndex, allPiecesIndex);
			} else if (len > 0) {
				ste_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
			}
		}
	}
}

Void ste_deleteSelection(ste_Buffer *buffer, Uint position, Uint len) {
	/* @todo @optimize
	 * ste__buffer_getPieceInfoFromPosition does the same work twice
	 */
	Uint count = 0;
	Uint firstPieceIndex = 0, lastPieceIndex = 0;
	Uint firstLen = 0, lastLen = 0;
	String8 firstStr = {0}, lastStr = {0};
	String8 newFirstStr = {0}, newLastStr = {0};

	buffer->mode = ste_BufferMode_None;

	ste__buffer_getPieceInfoFromPosition(buffer, position, &firstPieceIndex, &firstLen);
	ste__buffer_getPieceInfoFromPosition(buffer, position + len, &lastPieceIndex, &lastLen);
	ste__buffer_pieceGetString(buffer, firstPieceIndex, &firstStr);
	ste__buffer_pieceGetString(buffer, lastPieceIndex, &lastStr);

	for (count = 0; count < lastPieceIndex - firstPieceIndex + 1; count++) {
		ste__buffer_pieceRemove(buffer, firstPieceIndex);
	}

	newFirstStr = strSlice(firstStr, 0, firstLen);
	newLastStr = strSlice(lastStr, lastLen, lastStr.len - lastLen);

	if (newLastStr.len > 0) {
		ste_newPieceInsert(buffer, firstPieceIndex, newLastStr);
	}

	if (newFirstStr.len > 0) {
		ste_newPieceInsert(buffer, firstPieceIndex, newFirstStr);
	}
}

Int ste_deleteChar(ste_Buffer *buffer, Uint position) {
	Uint pieceIndex = 0, len = 0, lastBegin = 0;
	String8 str = {0};

	buffer->mode = ste_BufferMode_None;

	if (buffer->currentPieces.len <= 0) {
		return 1;
	} else {
		ste__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		ste__buffer_pieceGetString(buffer, pieceIndex, &str);
	
		ste__buffer_pieceRemove(buffer, pieceIndex);
		lastBegin = len + 1;
		if (lastBegin - str.len > 0) {
			ste_newPieceInsert(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));
		}
		if (len > 0) {
			ste_newPieceInsert(buffer, pieceIndex, strSlice(str, 0, len));
		}

		return 0;
	}
}

ste_Action ste__action(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex) {
	ste_Action action = {0};
	action.currentPiecesIndex = currentPiecesIndex;
	action.allPiecesBeforeIndex = allPiecesBeforeIndex;
	action.allPiecesAfterIndex = allPiecesAfterIndex;
	return action;
}

ste_Action ste_makeUndoAction(Void) {
	return ste__action((Uint)-1, 0, 0);
}

ste_Action ste_makeReplaceAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesBeforeIndex != (Uint)-1);
	assert(allPiecesAfterIndex != (Uint)-1);
	return ste__action(currentPiecesIndex, allPiecesBeforeIndex, allPiecesAfterIndex);
}

ste_Action ste_makeInsertAction(Uint currentPiecesIndex, Uint allPiecesAfterIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesAfterIndex != (Uint)-1);
	return ste__action(currentPiecesIndex, (Uint)-1, allPiecesAfterIndex);
}

ste_Action ste_makeRemoveAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesBeforeIndex != (Uint)-1);
	return ste__action(currentPiecesIndex, allPiecesBeforeIndex, (Uint)-1);
}

Bool ste_actionIsUndo(ste_Action *action) {
	return action->currentPiecesIndex == (Uint)-1;
}

Bool ste_actionIsReplace(
	ste_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex,
	Uint *allPiecesAfterIndex
) {
	if (action->currentPiecesIndex != (Uint)-1 && currentPiecesIndex != NULL) {
		*currentPiecesIndex = action->currentPiecesIndex;
	}

	if (action->allPiecesBeforeIndex != (Uint)-1 && allPiecesBeforeIndex != NULL) {
		*allPiecesBeforeIndex = action->allPiecesBeforeIndex;
	}

	if (action->allPiecesBeforeIndex != (Uint)-1 && allPiecesBeforeIndex != NULL) {
		*allPiecesAfterIndex = action->allPiecesAfterIndex;
	}

	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->allPiecesBeforeIndex != (Uint)-1 &&
		action->allPiecesAfterIndex != (Uint)-1;
}

Bool ste_actionIsInsert(
	ste_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex
) {
	if (action->currentPiecesIndex != (Uint)-1 && currentPiecesIndex != NULL) {
		*currentPiecesIndex = action->currentPiecesIndex;
	}

	if (action->allPiecesBeforeIndex != (Uint)-1 && allPiecesBeforeIndex != NULL) {
		*allPiecesBeforeIndex = action->allPiecesBeforeIndex;
	}

	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->allPiecesBeforeIndex == (Uint)-1 &&
		action->allPiecesAfterIndex != (Uint)-1;
}

Bool ste_actionIsRemove(
	ste_Action *action,
	Uint *currentPiecesIndex
) {
	if (action->currentPiecesIndex != (Uint)-1 && currentPiecesIndex != NULL) {
		*currentPiecesIndex = action->currentPiecesIndex;
	}

	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->allPiecesBeforeIndex != (Uint)-1 &&
		action->allPiecesAfterIndex == (Uint)-1;
}
