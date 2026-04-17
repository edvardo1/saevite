#if !defined(STE_TEXT_H)
#define STE_TEXT_H

#include "acyacsl.h"

typedef struct saevite_Buffer saevite_Buffer;
typedef struct saevite_Action saevite_Action;
typedef struct saevite_Cursor saevite_Cursor;

struct saevite_Action {
	Uint currentPiecesIndex;
	Uint before;
	Uint after;
};

struct saevite_Cursor {
	Int position;
	Uint clipboardRegisterIndex; /* reserved */

	Uint lastPosition;
	Uint lastCharAllPiecesIndex;
};

struct saevite_Buffer {
	DynamicArray(saevite_Cursor) cursors;
	DynamicArray(String8) allPieces;
	DynamicArray(Uint) currentPieces;
	DynamicArray(saevite_Action) actions;
	Uint actionsTop;

	String8 name;
};

Void saevite_buffer_init(saevite_Buffer *buffer);
Void saevite_printBuffer(const saevite_Buffer *buffer);
Void saevite_printBufferContents(const saevite_Buffer *buffer);
Void saevite_stringFromBuffer(const saevite_Buffer *buffer, String8 *string);

Int saevite_buffer_getCursorAmount(const saevite_Buffer *buffer);
Void saevite_buffer_getCursorPosition(const saevite_Buffer *buffer, Uint index, Uint *position);
Void saevite_buffer_setCursorPosition(saevite_Buffer *buffer, Uint index, Uint position);

Void saevite_pieceNew(saevite_Buffer *buffer, String8 str, Uint *index);
Void saevite_undoSingle(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_redoSingle(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_undo(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_redo(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_insertString(saevite_Buffer *buffer, Uint position, String8 str);
Void saevite_insertChar(saevite_Buffer *buffer, Int cursorIndex, Uint position, Char c);
Void saevite_deleteSelection(saevite_Buffer *buffer, Uint position, Uint len);
Int saevite_deleteChar(saevite_Buffer *buffer, Uint position);

saevite_Action saevite__action(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex);
saevite_Action saevite_makeUndoMarkerAction(Void);
saevite_Action saevite_makeReplaceAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex);
saevite_Action saevite_makeInsertAction(Uint currentPiecesIndex, Uint allPiecesAfterIndex);
saevite_Action saevite_makeRemoveAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex);
Bool saevite_actionIsUndoMarker(const saevite_Action *action);
Bool saevite_actionIsReplace(const saevite_Action *action);
Bool saevite_actionIsInsert(const saevite_Action *action);
Bool saevite_actionIsRemove(const saevite_Action *action);
Void saevite_buffer_addUndoMarkerIfNecessary(saevite_Buffer *buffer);

#endif /* !defined(STE_TEXT_H) */
