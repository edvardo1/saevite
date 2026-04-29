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
//Void saevite__actionCursor(const saevite_Buffer *buffer, const saevite_Action *action, Int *cursorPosition);
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
Bool saevite_action_isUndoMarker(saevite_Action *action);

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
	for (
		i = MAX(0, (Int)buffer->actions.len - 10);
		i < buffer->actions.len;
		i += 1
	) {
		saevite_Action *action = &buffer->actions.items[i];

		if (i + 1 < buffer->actionsTop) {
			printf(" o");
		} else if (buffer->actionsTop == i + 1) {
			printf(">>");
		} else {
			printf("  ");
		}

		switch (action->kind) {
		case saevite_ActionKind_UndoMarker:
			printf("[%ld] = undo marker\n", i);
			break;
		case saevite_ActionKind_Insert:
			printf("[%ld] = insert: ", i);
			printPieceString(buffer->allPieces.items[action->data.insert.after]);
			printf(" in %d\n", action->data.insert.index);
			break;
		case saevite_ActionKind_Remove:
			printf("[%ld] = remove: ", i);
			printPieceString(buffer->allPieces.items[action->data.remove.before]);
			printf(" in %d\n", action->data.remove.index);
			break;
		case saevite_ActionKind_Replace:
			printf("[%ld] = replace: ", i);
			printPieceString(buffer->allPieces.items[action->data.replace.before]);
			printf(" to ");
			printPieceString(buffer->allPieces.items[action->data.replace.after]);
			printf(" in %d\n", action->data.replace.index);
			break;
		case saevite_ActionKind_MoveCursor:
			printf(
				"[%ld] = move cursor[%d]: prev: %d, next: %d\n",
				i,
				action->data.moveCursor.index,
				action->data.moveCursor.previousPosition,
				action->data.moveCursor.nextPosition
			);
			break;
		case saevite_ActionKind_AddCursor:
			printf("[%ld] = add cursor: @todo\n", i);
			break;
		case saevite_ActionKind_RemoveCursor:
			printf("[%ld] = remove cursor: @todo\n", i);
			break;
		default:
			printf("unknown\n");
			break;
		}
	}

	printf("total len: %d\n", buffer->len);
	for (i = 0; i < buffer->cursors.len; i += 1) {
		printf("cursor[%ld]:\n", i);
		printf("  ");
		printf("position: %d\n", buffer->cursors.items[i].position);
		printf("  ");
		printf("clipboardRegisterIndex: %d\n", buffer->cursors.items[i].clipboardRegisterIndex);
		printf("  ");
		printf("mode: %d\n", buffer->cursors.items[i].mode);
		printf("  ");
		printf("lastPosition: %d\n", buffer->cursors.items[i].lastPosition);
		printf("  ");
		printf("lastCharAllPiecesIndex: %d\n", buffer->cursors.items[i].lastCharAllPiecesIndex);
		printf("  ");
		printf("lastActionIndex: %d\n", buffer->cursors.items[i].lastActionIndex);
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

Int saevite_buffer_getCursorAmount(const saevite_Buffer *buffer) {
	return buffer->cursors.len;
}

Void saevite_buffer_getCursorPosition(const saevite_Buffer *buffer, Uint index, Int *position) {
	assert(position != NULL);
	assert(index < buffer->cursors.len);
	*position = buffer->cursors.items[index].position;
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

Void saevite_buffer_pieceNew(saevite_Buffer *buffer, String8 str, Uint *index) {
	daAppend(&buffer->allPieces, str);
	if (index != NULL) {
		*index = buffer->allPieces.len - 1;
	}
}

void saevite__doAction(saevite_Buffer *buffer, const saevite_Action *action) {
	switch (action->kind) {
	case saevite_ActionKind_Replace:
		assert(
			buffer->currentPieces.items[action->data.replace.index] ==
			action->data.replace.before
		);
		buffer->currentPieces.items[action->data.replace.index] =
			action->data.replace.after;
		buffer->len -= buffer->allPieces.items[action->data.replace.before].len;
		buffer->len += buffer->allPieces.items[action->data.replace.after].len;
		break;
	case saevite_ActionKind_Insert:
		daAppendZ(&buffer->currentPieces);
		memmove(
			&buffer->currentPieces.items[action->data.insert.index + 1],
			&buffer->currentPieces.items[action->data.insert.index],
			(buffer->currentPieces.len - 1 - action->data.insert.index) *
			sizeof(*buffer->currentPieces.items)
		);
		buffer->currentPieces.items[action->data.insert.index] =
			action->data.insert.after;
		buffer->len += buffer->allPieces.items[action->data.insert.after].len;
		break;
	case saevite_ActionKind_Remove:
		memmove(
			&buffer->currentPieces.items[action->data.remove.index],
			&buffer->currentPieces.items[action->data.remove.index + 1],
			(buffer->currentPieces.len - 1 - action->data.remove.index) *
			sizeof(*buffer->currentPieces.items)
		);
		buffer->currentPieces.len -= 1;
		buffer->len -= buffer->allPieces.items[action->data.remove.before].len;
		break;
	case saevite_ActionKind_MoveCursor:
		assert(action->data.moveCursor.index < buffer->cursors.len);
		if (
			buffer->cursors.items[action->data.moveCursor.index].position !=
			action->data.moveCursor.previousPosition
		) {
			printf(
				"%d != %d\n",
				buffer->cursors.items[action->data.moveCursor.index].position,
				action->data.moveCursor.previousPosition
			);
			abort();
		}
		buffer->cursors.items[action->data.moveCursor.index].position =
			action->data.moveCursor.nextPosition;
		break;
	default:
		assert(0);
	}
}

void saevite__actionReverse(saevite_Action *dst, const saevite_Action *src) {
	saevite_Action src_copy = *src;

	switch (src_copy.kind) {
	case saevite_ActionKind_Replace:
		dst->kind = saevite_ActionKind_Replace;
		dst->data.replace.index  = src_copy.data.replace.index;
		dst->data.replace.before = src_copy.data.replace.after;
		dst->data.replace.after  = src_copy.data.replace.before;
		break;
	case saevite_ActionKind_Insert:
		dst->kind = saevite_ActionKind_Remove;
		dst->data.remove.index  = src_copy.data.insert.index;
		dst->data.remove.before = src_copy.data.insert.after;
		break;
	case saevite_ActionKind_Remove:
		dst->kind = saevite_ActionKind_Insert;
		dst->data.insert.index = src_copy.data.remove.index;
		dst->data.insert.after = src_copy.data.remove.before;
		break;
	case saevite_ActionKind_MoveCursor:
		dst->kind = saevite_ActionKind_MoveCursor;
		dst->data.moveCursor.index            = src_copy.data.moveCursor.index;
		dst->data.moveCursor.previousPosition = src_copy.data.moveCursor.nextPosition;
		dst->data.moveCursor.nextPosition     = src_copy.data.moveCursor.previousPosition;
		break;
	case saevite_ActionKind_AddCursor:
		dst->kind = saevite_ActionKind_RemoveCursor;
		dst->data.removeCursor.index    = src_copy.data.addCursor.index;
		dst->data.removeCursor.position = src_copy.data.addCursor.position;
		break;
	case saevite_ActionKind_RemoveCursor:
		dst->kind = saevite_ActionKind_AddCursor;
		dst->data.addCursor.index    = src_copy.data.removeCursor.index;
		dst->data.addCursor.position = src_copy.data.removeCursor.position;
		break;
	default:
		assert(0);
	}
}

Void saevite_buffer_undoSingle(saevite_Buffer *buffer) {
	const saevite_Action *action = NULL;
	saevite_Action reversedAction = {0};

	if (buffer->actionsTop > 0) {
		action = &buffer->actions.items[buffer->actionsTop - 1];
		buffer->actionsTop -= 1;

		saevite__actionReverse(&reversedAction, action);
		saevite__doAction(buffer, &reversedAction);
	}
}

Void saevite_buffer_redoSingle(saevite_Buffer *buffer) {
	saevite_Action *action = NULL;

	if (buffer->actionsTop < buffer->actions.len) {
		action = &buffer->actions.items[buffer->actionsTop];
		buffer->actionsTop += 1;

		saevite__doAction(buffer, action);
	}
}

Void saevite_buffer_undo(saevite_Buffer *buffer) {
	if (
		buffer->actionsTop > 0 &&
		saevite_action_isUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])
	) {
		buffer->actionsTop -= 1;
	}

	while (
		buffer->actionsTop > 0 &&
		!saevite_action_isUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])
	) {
		saevite_buffer_undoSingle(buffer);
	}
}

Void saevite_buffer_redo(saevite_Buffer *buffer) {
	if (buffer->actionsTop == buffer->actions.len) {
		return;
	}

	if (saevite_action_isUndoMarker(&buffer->actions.items[buffer->actionsTop])) {
		buffer->actionsTop += 1;
	}

	while (
		buffer->actionsTop < buffer->actions.len &&
		!saevite_action_isUndoMarker(&buffer->actions.items[buffer->actionsTop])
	) {
		saevite_buffer_redoSingle(buffer);
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
		saevite_action_insert(
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
		saevite_action_replace(
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
		saevite_action_remove(
			currentPiecesPosition,
			buffer->currentPieces.items[currentPiecesPosition]
		)
	);
	buffer->actionsTop = buffer->actions.len;

	saevite__doAction(buffer, &buffer->actions.items[buffer->actions.len - 1]);
}

Void saevite__buffer_newPieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	saevite_buffer_pieceNew(buffer, string, &pieceIndex);
	saevite__buffer_pieceInsert(buffer, currentPiecesPosition, pieceIndex);
}

Void saevite__buffer_newPieceReplace(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string) {
	Uint pieceIndex = 0;
	saevite_buffer_pieceNew(buffer, string, &pieceIndex);
	saevite__buffer_pieceReplace(buffer, currentPiecesPosition, pieceIndex);
}

Void saevite_buffer_insertString(saevite_Buffer *buffer, Uint position, String8 str) {
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

Void saevite_buffer_insertChar(saevite_Buffer *buffer, Int cursorIndex, Uint position, Char c) {
	/*
	 * @todo
	 * refactor this function, there is a lot of duplicated code
	 * here for dealing with the piece optimizations
	 */
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
		((action->kind == saevite_ActionKind_Insert &&
		  action->data.insert.after == cursor->lastCharAllPiecesIndex) ||
		 (action->kind == saevite_ActionKind_Replace &&
		  action->data.replace.after == cursor->lastCharAllPiecesIndex))
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
		buffer->len += 1;
	} else {
		str.buf = malloc(1);
		assert(str.buf != NULL);
		str.buf[0] = c;
		str.len = 1;
		saevite_buffer_pieceNew(buffer, str, &cpIndex);

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

Void saevite_buffer_deleteSelection(saevite_Buffer *buffer, Uint position, Uint len) {
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

Int saevite_buffer_deleteChar(saevite_Buffer *buffer, Int cursorIndex, Uint position) {
	/*
	 * @todo
	 * refactor this function, there is a lot of duplicated code
	 * here for dealing with the piece optimizations
	 */
	Uint pieceIndex = 0, len = 0, lastBegin = 0;
	String8 str = {0};
	saevite_Cursor *cursor = &buffer->cursors.items[cursorIndex];
	const saevite_Action *action = &buffer->actions.items[cursor->lastActionIndex];

	if (buffer->currentPieces.len <= 0) {
		return 1;
	} else if (
		cursor->mode == saevite_CursorMode_DeletingChars &&
		position == cursor->lastPosition - 1 &&
		((action->kind == saevite_ActionKind_Insert &&
		  action->data.insert.after == cursor->lastCharAllPiecesIndex) ||
		 (action->kind == saevite_ActionKind_Replace &&
		  action->data.replace.after == cursor->lastCharAllPiecesIndex)) &&
		buffer->allPieces.items[cursor->lastCharAllPiecesIndex].len > 1
	) {
		buffer->allPieces.items[cursor->lastCharAllPiecesIndex].len -= 1;
		cursor->lastPosition = position;
		buffer->len -= 1;

		return 0;
	} else {
		saevite__buffer_getPieceInfoFromPosition(buffer, position, &pieceIndex, &len);
		saevite__buffer_pieceGetString(buffer, pieceIndex, &str);
		lastBegin = len + 1;
	
		if (lastBegin - str.len > 0 && len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));

			cursor->mode = saevite_CursorMode_DeletingChars;
			cursor->lastPosition = position;
			cursor->lastActionIndex = buffer->actionsTop - 1;
			cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];

			saevite__buffer_newPieceInsert(buffer, pieceIndex, strSlice(str, 0, len));
		} else if (lastBegin - str.len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, lastBegin, str.len - lastBegin));

			cursor->mode = saevite_CursorMode_DeletingChars;
			cursor->lastPosition = position;
			cursor->lastActionIndex = buffer->actionsTop - 1;
			cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];
		} else if (len > 0) {
			saevite__buffer_newPieceReplace(buffer, pieceIndex, strSlice(str, 0, len));

			cursor->mode = saevite_CursorMode_DeletingChars;
			cursor->lastPosition = position;
			cursor->lastActionIndex = buffer->actionsTop - 1;
			cursor->lastCharAllPiecesIndex = buffer->currentPieces.items[pieceIndex];
		} else {
			saevite__buffer_pieceRemove(buffer, pieceIndex);
		}

		return 0;
	}
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

	if (!saevite_action_isUndoMarker(&buffer->actions.items[buffer->actionsTop - 1])) {
		daAppend(&buffer->actions, saevite_action_undoMarker());
		buffer->actionsTop += 1;
	}
}

Void saevite_buffer_cursorMoveRelative(saevite_Buffer *buffer, Uint index, Int offset) {
	U32 newPosition = buffer->cursors.items[index].position + offset;
	saevite_buffer_cursorMoveAbsolute(buffer, index, newPosition);
}

Void saevite_buffer_cursorMoveAbsolute(saevite_Buffer *buffer, Uint index, Uint position) {
	U32 lastPosition = buffer->cursors.items[index].position;
	U32 newPosition = position;
	daAppend(
		&buffer->actions,
		saevite_action_moveCursor(index, lastPosition, newPosition)
	);
	buffer->actionsTop = buffer->actions.len;
	saevite__doAction(buffer, &buffer->actions.items[buffer->actions.len - 1]);
}

saevite_Action saevite_action_replace(U32 index, U32 before, U32 after) {
	saevite_Action action = {0};
	action.kind = saevite_ActionKind_Replace;
	action.data.replace.index = index;
	action.data.replace.before = before;
	action.data.replace.after = after;
	return action;
}

saevite_Action saevite_action_insert(U32 index, U32 after) {
	saevite_Action action = {0};
	action.kind = saevite_ActionKind_Insert;
	action.data.insert.index = index;
	action.data.insert.after = after;
	return action;
}

saevite_Action saevite_action_remove(U32 index, U32 before) {
	saevite_Action action = {0};
	action.kind = saevite_ActionKind_Remove;
	action.data.remove.index = index;
	action.data.remove.before = before;
	return action;
}

saevite_Action saevite_action_moveCursor(U32 index, U32 previousPosition, U32 nextPosition) {
	saevite_Action action = {0};
	action.kind = saevite_ActionKind_MoveCursor;
	action.data.moveCursor.index = index;
	action.data.moveCursor.previousPosition = previousPosition;
	action.data.moveCursor.nextPosition = nextPosition;
	return action;
}

saevite_Action saevite_action_undoMarker() {
	saevite_Action action = {0};
	action.kind = saevite_ActionKind_UndoMarker;
	return action;
}

Bool saevite_action_isUndoMarker(saevite_Action *action) {
	return action->kind == saevite_ActionKind_UndoMarker;
}
