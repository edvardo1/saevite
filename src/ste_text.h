#if !defined(STE_TEXT_H)
#define STE_TEXT_H

#include "acyacsl.h"

typedef struct ste_Buffer ste_Buffer;
typedef struct ste_Action ste_Action;

struct ste_Action {
	Uint currentPiecesIndex;
	Uint allPiecesBeforeIndex;
	Uint allPiecesAfterIndex;
};

typedef enum ste_BufferMode {
	ste_BufferMode_None,
	ste_BufferMode_InsertingChars,
	ste_BufferMode_BackDeletingChars,
	ste_BufferMode_FrontDeletingChars,
} ste_BufferMode;

struct ste_Buffer {
	DynamicArray(String8) allPieces;
	DynamicArray(Uint) currentPieces;
	DynamicArray(ste_Action) actions;
	Uint lastAction; /* @todo unused for now */

	ste_BufferMode mode;
	Uint lastPosition;
	Uint lastCharAllPiecesIndex;
};

Void ste_printBuffer(ste_Buffer *buffer);
Void ste_stringFromBuffer(ste_Buffer *buffer, String8 *string);
Void ste_printBufferContents(ste_Buffer *buffer);
Int ste__buffer_getPieceInfoFromPosition(ste_Buffer *buffer, Uint position, Uint *pieceIndex, Uint *len);
Void ste_pieceNew(ste_Buffer *buffer, String8 str, Uint *index);
Void ste_pieceInsert(ste_Buffer *buffer, Uint currentPiecesPosition, Uint allPiecesIndex);
Void ste_newPieceInsert(ste_Buffer *buffer, Uint currentPiecesPosition, String8 string);
Void ste__buffer_pieceGetString(ste_Buffer *buffer, Uint currentPiecesPosition, String8 *str);
Void ste__buffer_pieceReplace(
	ste_Buffer *buffer,
	Uint currentPiecesPosition,
	Uint allPiecesIndex
);
Void ste__buffer_newPieceReplace(ste_Buffer *buffer, Uint currentPiecesPosition, String8 string);
Void ste__buffer_pieceRemove(ste_Buffer *buffer, Uint currentPiecesPosition);
Void ste_insertString(ste_Buffer *buffer, Uint position, String8 str);
Void ste_insertChar(ste_Buffer *buffer, Uint position, Char c);
Void ste_deleteSelection(ste_Buffer *buffer, Uint position, Uint len);
Int ste_deleteChar(ste_Buffer *buffer, Uint position);

ste_Action ste_makeUndoAction(Void);
ste_Action ste_makeReplaceAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex, Uint allPiecesAfterIndex);
ste_Action ste_makeInsertAction(Uint currentPiecesIndex, Uint allPiecesAfterIndex);
ste_Action ste_makeRemoveAction(Uint currentPiecesIndex, Uint allPiecesBeforeIndex);
Bool ste_actionIsUndo(ste_Action *action);
Bool ste_actionIsReplace(
	ste_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex,
	Uint *allPiecesAfterIndex
);
Bool ste_actionIsInsert(
	ste_Action *action,
	Uint *currentPiecesIndex,
	Uint *allPiecesBeforeIndex
);
Bool ste_actionIsRemove(
	ste_Action *action,
	Uint *currentPiecesIndex
);

#endif /* !defined(STE_TEXT_H) */
