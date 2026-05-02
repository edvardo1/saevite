#if !defined(SAEVITE_TEXT_H)
#define SAEVITE_TEXT_H

#include "acyacsl.h"

typedef struct saevite_Buffer saevite_Buffer;
typedef struct saevite_Action saevite_Action;
typedef struct saevite_Cursor saevite_Cursor;

typedef enum {
	saevite_ActionKind_Replace,
	saevite_ActionKind_Insert,
	saevite_ActionKind_Remove,
	saevite_ActionKind_MoveCursor,
	saevite_ActionKind_AddCursor,
	saevite_ActionKind_RemoveCursor,
	saevite_ActionKind_UndoMarker,
	saevite_ActionKind_COUNT
} saevite_ActionKind;

struct saevite_Action {
	U32 kind;
	union {
		struct {
			U32 index;
			U32 before;
			U32 after;
		} replace;
		struct {
			U32 index;
			U32 after;
		} insert;
		struct {
			U32 index;
			U32 before;
		} remove;
		struct {
			U32 index;
			U32 previousPosition;
			U32 nextPosition;
		} moveCursor;
		struct {
			U32 index;
			U32 position;
		} addCursor;
		struct {
			U32 index;
			U32 position;
		} removeCursor;
	} data;
};

typedef enum {
	saevite_CursorMode_None           = 0x0,
	saevite_CursorMode_InsertingChars = 0x1,
	saevite_CursorMode_DeletingChars  = 0x2,
	saevite_CursorMode_Moving         = 0x4,
} saevite_CursorMode;

struct saevite_Cursor {
	U32 position;
	Uint clipboardRegisterIndex; /* reserved */

	U32 mode;
	Uint lastPosition;
	Uint lastCharAllPiecesIndex;
	Uint lastActionIndex;
	Uint lastMoveActionIndex;
	Uint lastMovePosition;
};

struct saevite_Buffer {
	/*
	 * @todo
	 * store all of the string contents in here
	 * currently, the strings are malloc'ed randomly by the functions that
	 * deal with the buffer, that shouldn't happen
	 *
	 * we could have a buffer for all the bytes and have a freelist
	 * in case some parts end up not being used due to the undo feature
	 */
	/*
	 * @todo
	 * rename allPieces and currentPieces, as they have confusing names
	 *
	 * allPieces is the array where all the pieces actually are
	 * currentPieces is an array of indices into allPieces
	 *
	 * maybe rename them to pieces and pieceIndices?
	 */
	/*
	 * @todo?
	 * make allPieces a DynamicArray(Piece) instead of DynamicArray(String8)
	 *
	 * String8 was fine for a while but storing an extra boolean for whether
	 * the string is a string slice or if it is actually an allocateed string
	 */
	DynamicArray(saevite_Cursor) cursors;
	DynamicArray(String8) allPieces;
	DynamicArray(Uint) currentPieces;
	DynamicArray(saevite_Action) actions;
	Uint len;
	Uint actionsTop;

	String8 name;
};

Void saevite_buffer_init(saevite_Buffer *buffer);
Void saevite_printBuffer(const saevite_Buffer *buffer, Int actionsPrinted);
Void saevite_printBufferContents(const saevite_Buffer *buffer);
Void saevite_stringFromBuffer(const saevite_Buffer *buffer, String8 *string);

Int saevite_buffer_getCursorAmount(const saevite_Buffer *buffer);
Void saevite_buffer_getCursorPosition(const saevite_Buffer *buffer, Uint index, Int *position);

Void saevite_buffer_pieceNew(saevite_Buffer *buffer, String8 str, Uint *index);
Void saevite_buffer_undoSingle(saevite_Buffer *buffer);
Void saevite_buffer_redoSingle(saevite_Buffer *buffer);
Void saevite_buffer_undo(saevite_Buffer *buffer);
Void saevite_buffer_redo(saevite_Buffer *buffer);

Void saevite_buffer_cursorInsertString(saevite_Buffer *buffer, Uint cursorIndex, String8 str);
Void saevite_buffer_cursorInsertChar(saevite_Buffer *buffer, Int cursorIndex, Char c);
Void saevite_buffer_cursorDeleteSelection(saevite_Buffer *buffer, Uint cursorIndex, Uint len);
Int  saevite_buffer_cursorDeleteChar(saevite_Buffer *buffer, Int cursorIndex);

Void saevite_buffer_addUndoMarkerIfNecessary(saevite_Buffer *buffer);

Void saevite_buffer_cursorMoveRelative(saevite_Buffer *buffer, Uint index, Int offset);
Void saevite_buffer_cursorMoveAbsolute(saevite_Buffer *buffer, Uint index, Uint position);

saevite_Action saevite_action_replace(U32 index, U32 before, U32 after);
saevite_Action saevite_action_insert(U32 index, U32 after);
saevite_Action saevite_action_remove(U32 index, U32 before);
saevite_Action saevite_action_undoMarker(Void);
saevite_Action saevite_action_moveCursor(U32 index, U32 previousPosition, U32 nextPosition);

#endif /* !defined(SAEVITE_TEXT_H) */
