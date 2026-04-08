#if !defined(STE_TEXT_H)
#define STE_TEXT_H

#include "acyacsl.h"

typedef struct saevite_Buffer saevite_Buffer;
typedef struct saevite_Action saevite_Action;

struct saevite_Action {
	Uint currentPiecesIndex;
	Uint allPiecesBeforeIndex;
	Uint allPiecesAfterIndex;
};

typedef enum saevite_BufferMode {
	saevite_BufferMode_None,
	saevite_BufferMode_InsertingChars,
	saevite_BufferMode_BackDeletingChars,
	saevite_BufferMode_FrontDeletingChars,
} saevite_BufferMode;

struct saevite_Buffer {
	DynamicArray(String8) allPieces;
	DynamicArray(Uint) currentPieces;
	DynamicArray(saevite_Action) actions;
	Uint lastActionIndex;

	saevite_BufferMode mode;

	Uint lastPosition;
	Uint lastCharAllPiecesIndex;
	Bool doMergeInsertedChars;
};

Void saevite_printBuffer(saevite_Buffer *buffer);
Void saevite_stringFromBuffer(saevite_Buffer *buffer, String8 *string);
Void saevite_printBufferContents(saevite_Buffer *buffer);
Int saevite__buffer_getPieceInfoFromPosition(saevite_Buffer *buffer, Uint position, Uint *pieceIndex, Uint *len);
Void saevite_pieceNew(saevite_Buffer *buffer, String8 str, Uint *index);
Void saevite_undoSingle(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_redoSingle(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_undo(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_redo(saevite_Buffer *buffer, Int *cursorPosition);
Void saevite_pieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, Uint allPiecesIndex);
Void saevite_newPieceInsert(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string);
Void saevite__buffer_pieceGetString(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 *str);
Void saevite__buffer_pieceReplace(
	saevite_Buffer *buffer,
	Uint currentPiecesPosition,
	Uint allPiecesIndex
);
Void saevite__buffer_newPieceReplace(saevite_Buffer *buffer, Uint currentPiecesPosition, String8 string);
Void saevite__buffer_pieceRemove(saevite_Buffer *buffer, Uint currentPiecesPosition);
Void saevite_insertString(saevite_Buffer *buffer, Uint position, String8 str);
Void saevite_insertChar(saevite_Buffer *buffer, Uint position, Char c);
Void saevite_deleteSelection(saevite_Buffer *buffer, Uint position, Uint len);
Int saevite_deleteChar(saevite_Buffer *buffer, Uint position);

saevite_Action saevite_makeUndoAction(Void);
saevite_Action saevite_makeReplaceAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex);
saevite_Action saevite_makeInsertAction(Uint currentPiecesIndex, Uint allPiecesAfterIndex);
saevite_Action saevite_makeRemoveAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex);
Bool saevite_actionIsUndo(saevite_Action *action);
Bool saevite_actionIsReplace(
	saevite_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex,
	Uint *allPiecesAfterIndex
);
Bool saevite_actionIsInsert(
	saevite_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex
);
Bool saevite_actionIsRemove(
	saevite_Action *action,
	Uint *currentPiecesIndex
);

#endif /* !defined(STE_TEXT_H) */
