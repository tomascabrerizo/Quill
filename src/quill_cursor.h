#ifndef _QUILL_CURSOR_H_
#define _QUILL_CURSOR_H_

#include "quill.h"

typedef struct Cursor {
  u32 col;
  u32 save_col;
  u32 line;
} Cursor;

void cursor_print(Cursor cursor);
bool cursor_equals(Cursor a, Cursor b);

#endif /* _QUILL_CURSOR_H_ */
