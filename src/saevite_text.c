#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "saevite_text.h"

const Uint full = 0xffffffff;

Void printPieceString(String8 string);
Int saevite__buffer_getPieceInfoFromPosition(const saevite_Buffer *buffer, Uint position, Uint *pieceIndex, Uint *len);
Void saevite__doAction(saevite_Buffer *buffer, const saevite_Action *action);
Void saevite__actionReverse(saevite_Action *dst, const saevite_Action *src);
Void saevite__actionCursor(const saevite_Buffer *buffer, const saevite_Action *action, Int *cursorPosition);
Void saevite__buffer_pieceGetString(const saevite_Buffer *buffer, Uint currentPiecesPosition, String8 *str);
Void saevite__buffer_pieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, Uint allPiecesIndex);
Void saevite__buffer_pieceReplace(
	saevite_Buffer *buffer,
	Uint currentPiecesPosition,
	Uint allPiecesIndex
);
Void saevite__buffer_pieceRemove(saevite_Buffer *buffer, Uint currentPiecesPosition);
Void saevite__buffer_newPieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string);
Void saevite__buffer_newPieceReplace(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string);

Void saevite_buffer_init(saevite_Buffer *buffer) {
	daAppendZ(&buffer->cursors);
	buffer->cursors.items[buffer->cursors.len - 1].position = 0;
	buffer->cursors.items[buffer->cursors.len - 1].clipboardRegisterIndex = 0;
	buffer->name = S("*scratch*");
}

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

Void saevite_printBuffer(const saevite_Buffer *buffer) {
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
	printf("actions:\n");
	printf("  actionsTop: %u\n", buffer->actionsTop);
	for (i = 0; i < buffer->actions.len; i++) {
		saevite_Action *a = &buffer->actions.items[i];

		if (i + 1 < buffer->actionsTop) {
			printf(" o");
		} else if (buffer->actionsTop == i + 1) {
			printf(">>");
		} else {
			printf("  ");
		}

		if (a->currentPiecesIndex == full) {
			printf("[%ld] = undo or unknown\n", i);
		} else if (a->allPiecesBeforeIndex == full && a->allPiecesAfterIndex == full) {
			printf("[%ld] = invalid\n", i);
		} else if (a->allPiecesBeforeIndex == full && a->allPiecesAfterIndex != full) {
			printf("[%ld] = insert: ", i);
			printPieceString(buffer->allPieces.items[a->allPiecesAfterIndex]);
			printf(" in %d\n", a->currentPiecesIndex);
		} else if (a->allPiecesBeforeIndex != full && a->allPiecesAfterIndex == full) {
			printf("[%ld] = remove: ", i);
			printPieceString(buffer->allPieces.items[a->allPiecesBeforeIndex]);
			printf(" in %d\n", a->currentPiecesIndex);
		} else if (a->allPiecesBeforeIndex != full && a->allPiecesAfterIndex != full) {
			printf("[%ld] = replace: ", i);
			printPieceString(buffer->allPieces.items[a->allPiecesBeforeIndex]);
			printf(" to ");
			printPieceString(buffer->allPieces.items[a->allPiecesAfterIndex]);
			printf(" in %d\n", a->currentPiecesIndex);
		} else {
			assert(0);
		}
	}
	printf("\n");
	printf("\n");
}

Void saevite_printBufferContents(const saevite_Buffer *buffer) {
	Usize i = 0;
	for (i = 0; i < buffer->currentPieces.len; i++) {
		printf(
			"%.*s",
			Slens(buffer->allPieces.items[buffer->currentPieces.items[i]])
		);
	}
}

Void saevite_stringFromBuffer(const saevite_Buffer *buffer, String8 *string) {
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

Int saevite__buffer_getPieceInfoFromPosition(const saevite_Buffer *buffer, Uint position, Uint *pieceIndex, Uint *len) {
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

Void saevite_pieceNew(saevite_Buffer *buffer, String8 str, Uint *index) {
	daAppend(&buffer->allPieces, str);
	if (index != NULL) {
		*index = buffer->allPieces.len - 1;
	}
}

void saevite__doAction(saevite_Buffer *buffer, const saevite_Action *action) {
	if (action->currentPiecesIndex != full) {
		if (action->allPiecesBeforeIndex != full && action->allPiecesAfterIndex != full) {
			assert(
				buffer->currentPieces.items[action->currentPiecesIndex] ==
				action->allPiecesBeforeIndex
			);
			buffer->currentPieces.items[action->currentPiecesIndex] =
				action->allPiecesAfterIndex;
		} else if (action->allPiecesBeforeIndex != full) {
			memmove(
				&buffer->currentPieces.items[action->currentPiecesIndex],
				&buffer->currentPieces.items[action->currentPiecesIndex + 1],
				(buffer->currentPieces.len - 1 - action->currentPiecesIndex) *
				sizeof(*buffer->currentPieces.items)
			);
			buffer->currentPieces.len -= 1;
		} else if (action->allPiecesAfterIndex != full) {
			/* insert a */
			daAppendZ(&buffer->currentPieces);
			memmove(
				&buffer->currentPieces.items[action->currentPiecesIndex + 1],
				&buffer->currentPieces.items[action->currentPiecesIndex],
				(buffer->currentPieces.len - 1 - action->currentPiecesIndex) *
				sizeof(*buffer->currentPieces.items)
			);
			buffer->currentPieces.items[action->currentPiecesIndex] =
					action->allPiecesAfterIndex;
		} else {
			assert(0);
		}
	}
}

void saevite__actionReverse(saevite_Action *dst, const saevite_Action *src) {
	saevite_Action src_copy = *src;

	if (src_copy.currentPiecesIndex != full) {
		dst->currentPiecesIndex = src_copy.currentPiecesIndex;

		if (src_copy.allPiecesBeforeIndex != full && src_copy.allPiecesAfterIndex != full) {
			/* replace a b -> replace b a */
			dst->allPiecesAfterIndex = src_copy.allPiecesBeforeIndex;
			dst->allPiecesBeforeIndex = src_copy.allPiecesAfterIndex;
		} else if (src_copy.allPiecesBeforeIndex != full) {
			/* remove a -> insert a */
			dst->allPiecesAfterIndex = src_copy.allPiecesBeforeIndex;
			dst->allPiecesBeforeIndex = full;
		} else if (src_copy.allPiecesAfterIndex != full) {
			/* insert a -> remove a */
			dst->allPiecesAfterIndex = full;
			dst->allPiecesBeforeIndex = src_copy.allPiecesAfterIndex;
		} else {
			assert(0);
		}
	}
}

Void saevite__actionCursor(const saevite_Buffer *buffer, const saevite_Action *action, Int *cursorPosition) {
	Uint cpIndex = 0;
	Uint pieceIndex = 0;
	Uint len = 0;
	Uint cpMaxIndex = 0;

	if (action->currentPiecesIndex == full) {
		return;
	}

	if (action->allPiecesAfterIndex == full) {
		cpMaxIndex = action->currentPiecesIndex;
	} else {
		cpMaxIndex = action->currentPiecesIndex + 1;
	}

	*cursorPosition = 0;
	for (cpIndex = 0; cpIndex < cpMaxIndex; cpIndex += 1) {
		pieceIndex = buffer->currentPieces.items[cpIndex];
		len = buffer->allPieces.items[pieceIndex].len;
		*cursorPosition += len;
	}
}

Void saevite_undoSingle(saevite_Buffer *buffer, Int *cursorPosition) {
	const saevite_Action *action = NULL;
	saevite_Action reversedAction = {0};

	if (buffer->actionsTop > 0) {
		action = &buffer->actions.items[buffer->actionsTop - 1];
		buffer->actionsTop -= 1;

		saevite__actionReverse(&reversedAction, action);
		saevite__doAction(buffer, &reversedAction);

		if (cursorPosition != NULL) {
			saevite__actionCursor(buffer, &reversedAction, cursorPosition);
		}
	}
}

Void saevite_redoSingle(saevite_Buffer *buffer, Int *cursorPosition) {
	saevite_Action *action = NULL;

	if (buffer->actionsTop < buffer->actions.len) {
		action = &buffer->actions.items[buffer->actionsTop];
		buffer->actionsTop += 1;

		saevite__doAction(buffer, action);

		if (cursorPosition != NULL) {
			saevite__actionCursor(buffer, action, cursorPosition);
		}
	}
}

Void saevite_undo(saevite_Buffer *buffer, Int *cursorPosition) {
	assert(0);
	UNUSED(buffer);
	UNUSED(cursorPosition);
	//while (buffer->actionsTop == 0) {
	//	saevite_undoSingle(buffer, cursorPosition);
	//}
}

Void saevite_redo(saevite_Buffer *buffer, Int *cursorPosition) {
	UNUSED(buffer);
	UNUSED(cursorPosition);

	assert(0);
}

Void saevite__buffer_pieceGetString(const saevite_Buffer *buffer, Uint currentPiecesPosition, String8 *str) {
	*str = buffer->allPieces.items[
		buffer->currentPieces.items[currentPiecesPosition]
	];
}

Void saevite__buffer_pieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, Uint allPiecesIndex) {
	buffer->actions.len = MIN(buffer->actionsTop, buffer->actions.len);
	daAppend(
		&buffer->actions,
		saevite_makeInsertAction(
			currentPiecesPosition,
			allPiecesIndex
		)
	);
	saevite__doAction(buffer, &buffer->actions.items[buffer->actions.len - 1]);
	buffer->actionsTop = buffer->actions.len;
}

Void saevite__buffer_pieceReplace(
	saevite_Buffer *buffer,
	Uint currentPiecesPosition,
	Uint allPiecesIndex
) {
	buffer->actions.len = MIN(buffer->actionsTop, buffer->actions.len);
	daAppend(
		&buffer->actions,
		saevite_makeReplaceAction(
			currentPiecesPosition,
			buffer->currentPieces.items[currentPiecesPosition],
			allPiecesIndex
		)
	);
	buffer->actionsTop = buffer->actions.len;
	saevite__doAction(buffer, &buffer->actions.items[buffer->actions.len - 1]);
}

Void saevite__buffer_pieceRemove(saevite_Buffer *buffer, Uint currentPiecesPosition) {
	buffer->actions.len = MIN(buffer->actionsTop, buffer->actions.len);
	daAppend(
		&buffer->actions,
		saevite_makeRemoveAction(
			currentPiecesPosition,
			buffer->currentPieces.items[currentPiecesPosition]
		)
	);
	buffer->actionsTop = buffer->actions.len;

	saevite__doAction(buffer, &buffer->actions.items[buffer->actions.len - 1]);
}

Void saevite__buffer_newPieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	saevite_pieceNew(buffer, string, &pieceIndex);
	saevite__buffer_pieceInsert(buffer, currentPiecesPosition, pieceIndex);
}

Void saevite__buffer_newPieceReplace(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	saevite_pieceNew(buffer, string, &pieceIndex);
	saevite__buffer_pieceReplace(buffer, currentPiecesPosition, pieceIndex);
}

Void saevite_insertString(saevite_Buffer *buffer, Uint position, String8 str) {
	String8 oldStr = {0};
	Uint pieceIndex = 0, len = 0;

	buffer->mode = saevite_BufferMode_None;

	saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);

	if (pieceIndex < buffer->currentPieces.len) {
		saevite__buffer_pieceGetString(buffer, pieceIndex, &oldStr);

		if (oldStr.len - len > 0 && len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(oldStr, len, oldStr.len - len));
			saevite__buffer_newPieceInsert(buffer, pieceIndex, str);
			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
		} else if (oldStr.len - len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(oldStr, len, oldStr.len - len));
			saevite__buffer_newPieceInsert(buffer, pieceIndex, str);
		} else if (len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, str);
			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
		} else {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, str);
		}
	} else {
		saevite__buffer_newPieceInsert(buffer, pieceIndex, str);
	}
}

Void saevite_insertChar(saevite_Buffer *buffer, Uint position, Char c) {
	String8 oldStr = {0};
	String8 str = {0};
	Uint pieceIndex = 0, len = 0;
	Uint cpIndex = 0;
	Uint allPiecesIndex = 0;
	Int err = 0;

	if (
		buffer->doMergeInsertedChars &&
		buffer->mode == saevite_BufferMode_InsertingChars &&
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
		saevite_pieceNew(buffer, str, &cpIndex);

		if (buffer->doMergeInsertedChars) {
			buffer->mode = saevite_BufferMode_InsertingChars;
			buffer->lastPosition = position;
		}

		if (buffer->currentPieces.len > 0) {
			err = saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		}

		if (buffer->currentPieces.len <= 0 || err) {
			if (position == 0) {
				saevite__buffer_pieceInsert(buffer, 0, cpIndex);
				buffer->lastCharAllPiecesIndex = buffer->currentPieces.items[0];
			} else {
				saevite__buffer_pieceInsert(buffer, buffer->currentPieces.len, cpIndex);
				buffer->lastCharAllPiecesIndex = buffer->allPieces.len - 1;
			}
		} else {
			allPiecesIndex = buffer->currentPieces.items[pieceIndex];
			saevite__buffer_pieceGetString(buffer, pieceIndex, &oldStr);

			if (oldStr.len - len > 0) {
				if (len > 0) { /* in order to not replace a piece with itself */
					saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(oldStr, len, oldStr.len - len));
				}
			} else {
				saevite__buffer_pieceRemove(buffer, pieceIndex);
			}

			saevite__buffer_pieceInsert(buffer, pieceIndex, cpIndex);
			buffer->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];

			if (len == oldStr.len) {
				saevite__buffer_pieceInsert(buffer, pieceIndex, allPiecesIndex);
			} else if (len > 0) {
				saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
			}
		}
	}
}

Void saevite_deleteSelection(saevite_Buffer *buffer, Uint position, Uint len) {
	/* @todo @optimize
	 * saevite__buffer_getPieceInfoFromPosition does the same work twice
	 */
	Uint count = 0;
	Uint firstPieceIndex = 0, lastPieceIndex = 0;
	Uint firstLen = 0, lastLen = 0;
	String8 firstStr = {0}, lastStr = {0};
	String8 newFirstStr = {0}, newLastStr = {0};

	buffer->mode = saevite_BufferMode_None;

	saevite__buffer_getPieceInfoFromPosition(buffer, position, &firstPieceIndex, &firstLen);
	saevite__buffer_getPieceInfoFromPosition(buffer, position + len, &lastPieceIndex, &lastLen);
	saevite__buffer_pieceGetString(buffer, firstPieceIndex, &firstStr);
	saevite__buffer_pieceGetString(buffer, lastPieceIndex, &lastStr);

	for (count = 0; count < lastPieceIndex - firstPieceIndex + 1; count++) {
		saevite__buffer_pieceRemove(buffer, firstPieceIndex);
	}

	newFirstStr = strSlice(firstStr, 0, firstLen);
	newLastStr = strSlice(lastStr, lastLen, lastStr.len - lastLen);

	if (newLastStr.len > 0) {
		saevite__buffer_newPieceInsert(buffer, firstPieceIndex, newLastStr);
	}

	if (newFirstStr.len > 0) {
		saevite__buffer_newPieceInsert(buffer, firstPieceIndex, newFirstStr);
	}
}

Int saevite_deleteChar(saevite_Buffer *buffer, Uint position) {
	Uint pieceIndex = 0, len = 0, lastBegin = 0;
	String8 str = {0};

	buffer->mode = saevite_BufferMode_None;

	if (buffer->currentPieces.len <= 0) {
		return 1;
	} else {
		saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		saevite__buffer_pieceGetString(buffer, pieceIndex, &str);
	
		saevite__buffer_pieceRemove(buffer, pieceIndex);
		lastBegin = len + 1;
		if (lastBegin - str.len > 0) {
			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));
		}
		if (len > 0) {
			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(str, 0, len));
		}

		return 0;
	}
}

saevite_Action saevite__action(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex) {
	saevite_Action action = {0};
	action.currentPiecesIndex = currentPiecesIndex;
	action.allPiecesBeforeIndex = allPiecesBeforeIndex;
	action.allPiecesAfterIndex = allPiecesAfterIndex;
	return action;
}

saevite_Action saevite_makeUndoAction(Void) {
	return saevite__action((Uint)-1, 0, 0);
}

saevite_Action saevite_makeReplaceAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesBeforeIndex != (Uint)-1);
	assert(allPiecesAfterIndex != (Uint)-1);
	return saevite__action(currentPiecesIndex, allPiecesBeforeIndex, allPiecesAfterIndex);
}

saevite_Action saevite_makeInsertAction(Uint currentPiecesIndex, Uint allPiecesAfterIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesAfterIndex != (Uint)-1);
	return saevite__action(currentPiecesIndex, (Uint)-1, allPiecesAfterIndex);
}

saevite_Action saevite_makeRemoveAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(allPiecesBeforeIndex != (Uint)-1);
	return saevite__action(currentPiecesIndex, allPiecesBeforeIndex, (Uint)-1);
}

Bool saevite_actionIsUndo(const saevite_Action *action) {
	return action->currentPiecesIndex == (Uint)-1;
}

Bool saevite_actionIsReplace(
	const saevite_Action *action,
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

Bool saevite_actionIsInsert(
	const saevite_Action *action,
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

Bool saevite_actionIsRemove(
	const saevite_Action *action,
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
