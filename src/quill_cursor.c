#include "quill_cursor.h"

void cursor_print(Cursor cursor) {
  printf("line:%d, col:%d\n", cursor.line, cursor.col);
}

bool cursor_equals(Cursor a, Cursor b) {
  return (a.line == b.line) && (a.col == b.col) && (a.save_col == b.save_col);
}
