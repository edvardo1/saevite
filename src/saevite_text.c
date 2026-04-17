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
		saevite_Action *action = &buffer->actions.items[i];

		if (i + 1 < buffer->actionsTop) {
			printf(" o");
		} else if (buffer->actionsTop == i + 1) {
			printf(">>");
		} else {
			printf("  ");
		}

		if (saevite_actionIsUndoMarker(action)) {
			printf("[%ld] = undo marker, %d -> %d\n", i, action->before, action->after);
		} else if (action->before == full && action->after == full) {
			printf("[%ld] = invalid\n", i);
		} else if (action->before == full && action->after != full) {
			printf("[%ld] = insert: ", i);
			printPieceString(buffer->allPieces.items[action->after]);
			printf(" in %d\n", action->currentPiecesIndex);
		} else if (action->before != full && action->after == full) {
			printf("[%ld] = remove: ", i);
			printPieceString(buffer->allPieces.items[action->before]);
			printf(" in %d\n", action->currentPiecesIndex);
		} else if (action->before != full && action->after != full) {
			printf("[%ld] = replace: ", i);
			printPieceString(buffer->allPieces.items[action->before]);
			printf(" to ");
			printPieceString(buffer->allPieces.items[action->after]);
			printf(" in %d\n", action->currentPiecesIndex);
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

	if (cumPosition == 0 && pieceIndex != NULL) {
		*pieceIndex = buffer->currentPieces.len;
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
		if (action->before != full && action->after != full) {
			assert(
				buffer->currentPieces.items[action->currentPiecesIndex] ==
				action->before
			);
			buffer->currentPieces.items[action->currentPiecesIndex] =
				action->after;
		} else if (action->before != full) {
			memmove(
				&buffer->currentPieces.items[action->currentPiecesIndex],
				&buffer->currentPieces.items[action->currentPiecesIndex + 1],
				(buffer->currentPieces.len - 1 - action->currentPiecesIndex) *
				sizeof(*buffer->currentPieces.items)
			);
			buffer->currentPieces.len -= 1;
		} else if (action->after != full) {
			/* insert a */
			daAppendZ(&buffer->currentPieces);
			memmove(
				&buffer->currentPieces.items[action->currentPiecesIndex + 1],
				&buffer->currentPieces.items[action->currentPiecesIndex],
				(buffer->currentPieces.len - 1 - action->currentPiecesIndex) *
				sizeof(*buffer->currentPieces.items)
			);
			buffer->currentPieces.items[action->currentPiecesIndex] =
					action->after;
		} else {
			assert(0);
		}
	}
}

void saevite__actionReverse(saevite_Action *dst, const saevite_Action *src) {
	saevite_Action src_copy = *src;

	if (src_copy.currentPiecesIndex != full) {
		dst->currentPiecesIndex = src_copy.currentPiecesIndex;

		if (src_copy.before != full && src_copy.after != full) {
			/* replace a b -> replace b a */
			dst->after = src_copy.before;
			dst->before = src_copy.after;
		} else if (src_copy.before != full) {
			/* remove a -> insert a */
			dst->after = src_copy.before;
			dst->before = full;
		} else if (src_copy.after != full) {
			/* insert a -> remove a */
			dst->after = full;
			dst->before = src_copy.after;
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

	if (action->after == full) {
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

	if (saevite_actionIsReplace(action)) {
		cpMaxIndex = action->currentPiecesIndex + 1;
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
	if (
		buffer->actionsTop > 0 &&
		saevite_actionIsUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])
	) {
		buffer->actionsTop -= 1;
	}

	while (
		buffer->actionsTop > 0 &&
		!saevite_actionIsUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])
	) {
		saevite_undoSingle(buffer, cursorPosition);
	}
}

Void saevite_redo(saevite_Buffer *buffer, Int *cursorPosition) {
	if (buffer->actionsTop == buffer->actions.len) {
		return;
	}

	//assert(
	//	buffer->actionsTop == 0
	//);

	if (saevite_actionIsUndoMarker(&buffer->actions.items[buffer->actionsTop])) {
		buffer->actionsTop += 1;
	}

	while (
		buffer->actionsTop < buffer->actions.len &&
		!saevite_actionIsUndoMarker(&buffer->actions.items[buffer->actionsTop])
	) {
		saevite_redoSingle(buffer, cursorPosition);
	}
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
	Int err = 0;

	err = saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);

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
		assert(err);
		saevite__buffer_newPieceInsert(buffer, pieceIndex, str);
	}
}

Void saevite_insertChar(saevite_Buffer *buffer, Int cursorIndex, Uint position, Char c) {
	String8 oldStr = {0};
	String8 str = {0};
	Uint pieceIndex = 0, len = 0;
	Uint cpIndex = 0;
	Int err = 0;
	saevite_Cursor *cursor = &buffer->cursors.items[cursorIndex];
	const saevite_Action *action = &buffer->actions.items[cursor->lastActionIndex];

	if (
		cursor->mode == saevite_CursorMode_InsertingChars &&
		position == cursor->lastPosition + 1 &&
		(saevite_actionIsInsert(action) || saevite_actionIsReplace(action)) &&
		action->after == cursor->lastCharAllPiecesIndex
	) {
		buffer->allPieces.items[cursor->lastCharAllPiecesIndex].buf = 
			realloc(
				buffer->allPieces.items[cursor->lastCharAllPiecesIndex].buf,
				buffer->allPieces.items[cursor->lastCharAllPiecesIndex].len + 1
			);
		buffer->allPieces.items[cursor->lastCharAllPiecesIndex].len += 1;
		buffer->allPieces.items[cursor->lastCharAllPiecesIndex].buf[
			buffer->allPieces.items[cursor->lastCharAllPiecesIndex].len - 1
		] = c;
		cursor->lastPosition = position;
	} else {
		str.buf = malloc(1);
		assert(str.buf != NULL);
		str.buf[0] = c;
		str.len = 1;
		saevite_pieceNew(buffer, str, &cpIndex);

		cursor->mode = saevite_CursorMode_InsertingChars;
		cursor->lastPosition = position;

		if (buffer->currentPieces.len > 0) {
			err = saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		}

		if (buffer->currentPieces.len <= 0 || err) {
			if (position == 0) {
				saevite__buffer_pieceInsert(buffer, 0, cpIndex);
				cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[0];
				cursor->lastActionIndex = buffer->actionsTop - 1;
			} else {
				saevite__buffer_pieceInsert(buffer, buffer->currentPieces.len, cpIndex);
				cursor->lastActionIndex = buffer->actionsTop - 1;
				cursor->lastCharAllPiecesIndex = buffer->allPieces.len - 1;
			}
		} else {
			saevite__buffer_pieceGetString(buffer, pieceIndex, &oldStr);

			if (oldStr.len - len > 0) {
				saevite__buffer_newPieceInsert(buffer, pieceIndex + 1, strSlice(oldStr, len, oldStr.len - len));
			}

			if (len > 0) {
				saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(oldStr, 0, len));
				saevite__buffer_pieceReplace(buffer, pieceIndex + 1, cpIndex);
				cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex + 1];
				cursor->lastActionIndex = buffer->actionsTop - 1;
			} else {
				saevite__buffer_pieceReplace(buffer, pieceIndex, cpIndex);
				cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];
				cursor->lastActionIndex = buffer->actionsTop - 1;
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

	if (buffer->currentPieces.len <= 0) {
		return 1;
	} else {
		saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		saevite__buffer_pieceGetString(buffer, pieceIndex, &str);
		lastBegin = len + 1;
	
		if (lastBegin - str.len > 0 && len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));
			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(str, 0, len));
		} else if (lastBegin - str.len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));
		} else if (len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, 0, len));
		} else {
			saevite__buffer_pieceRemove(buffer, pieceIndex);
		}

		return 0;
	}
}

saevite_Action saevite__action(Uint currentPiecesIndex, Uint before, Uint after) {
	saevite_Action action = {0};
	action.currentPiecesIndex = currentPiecesIndex;
	action.before = before;
	action.after = after;
	return action;
}

saevite_Action saevite_makeUndoMarkerAction(Void) {
	return saevite__action((Uint)-1, 0, 0);
}

saevite_Action saevite_makeReplaceAction(Uint currentPiecesIndex, Uint before, Uint after) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(before != (Uint)-1);
	assert(after != (Uint)-1);
	return saevite__action(currentPiecesIndex, before, after);
}

saevite_Action saevite_makeInsertAction(Uint currentPiecesIndex, Uint after) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(after != (Uint)-1);
	return saevite__action(currentPiecesIndex, (Uint)-1, after);
}

saevite_Action saevite_makeRemoveAction(Uint currentPiecesIndex, Uint before) {
	assert(currentPiecesIndex != (Uint)-1);
	assert(before != (Uint)-1);
	return saevite__action(currentPiecesIndex, before, (Uint)-1);
}

Bool saevite_actionIsUndoMarker(const saevite_Action *action) {
	return action->currentPiecesIndex == (Uint)-1;
}

Bool saevite_actionIsReplace(const saevite_Action *action) {
	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->before != (Uint)-1 &&
		action->after != (Uint)-1;
}

Bool saevite_actionIsInsert(const saevite_Action *action) {
	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->before == (Uint)-1 &&
		action->after != (Uint)-1;
}

Bool saevite_actionIsRemove(const saevite_Action *action) {
	return
		action->currentPiecesIndex != (Uint)-1 &&
		action->before != (Uint)-1 &&
		action->after == (Uint)-1;
}

Void saevite_buffer_addUndoMarkerIfNecessary(saevite_Buffer *buffer) {
	Uint cursorIndex = 0;

	assert(buffer->actionsTop <= buffer->actions.len);
	if (buffer->actionsTop == 0) {
		return;
	}

	for (cursorIndex = 0; cursorIndex < buffer->cursors.len; cursorIndex += 1) {
		buffer->cursors.items[cursorIndex].mode = saevite_CursorMode_None;
	}

	if (!saevite_actionIsUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])) {
		daAppend(
			&buffer->actions,
			saevite_makeUndoMarkerAction()
		);
		buffer->actionsTop += 1;
	}
}
