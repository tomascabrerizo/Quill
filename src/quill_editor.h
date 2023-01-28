#ifndef _QUILL_EDITOR_H_
#define _QUILL_EDITOR_H_

#include "quill.h"
#include "quill_element.h"

typedef struct Cursor {
  QUILL_ELEMENT

  u32 col;
  u32 save_col;
  u32 line;
} Cursor;

void cursor_print(Cursor cursor);

typedef struct Editor {
  QUILL_ELEMENT

  struct File *file;
  Cursor cursor;
  u32 col_offset;
  u32 line_offset;

  bool selected;
  Cursor selection_mark;

  Rect rect;
  bool redraw;
  u32 redraw_line_start;
  u32 redraw_line_end;
} Editor;

struct Editor *editor_create(Element *parent);
void editor_step_cursor_right(Editor *editor);
void editor_step_cursor_left(Editor *editor);
void editor_step_cursor_up(Editor *editor);
void editor_step_cursor_down(Editor *editor);
void editor_cursor_insert(Editor *editor, u8 codepoint);
void editor_cursor_insert_new_line(Editor *editor);
void editor_cursor_remove(Editor *editor);
void editor_cursor_remove_right(Editor *editor);
void editor_remove_selection(Editor *editor);
void editor_update_selected(Editor *editor, bool selected);
bool editor_is_selected(Editor *editor, u32 line, u32 col);
void editor_draw_text(Painter *painter, Editor *editor);
void editor_redraw_lines(Editor *editor, u32 start, u32 end);


#endif /* _QUILL_EDITOR_H_ */
