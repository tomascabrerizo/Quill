#include "quill_cursor.h"

void cursor_print(Cursor cursor) {
  printf("line:%d, col:%d\n", cursor.line, cursor.col);
}
