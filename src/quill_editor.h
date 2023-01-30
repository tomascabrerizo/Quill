#ifndef _QUILL_EDITOR_H_
#define _QUILL_EDITOR_H_

#include "quill.h"
#include "quill_element.h"

struct Painter;

typedef struct Cursor {
  u32 col;
  u32 save_col;
  u32 line;
} Cursor;

void cursor_print(Cursor cursor);

/* TODO: Get good keycodes for the editor keys */

#define EDITOR_MOD_SHIFT_RIGHT 0x80000000;
#define EDITOR_MOD_SHIFT_LEFT 0xc0000000;
#define EDITOR_MOD_SHIFT (EDITOR_MOD_SHIFT_RIGHT|EDITOR_MOD_SHIFT_LEFT)

#define EDITOR_KEY_RIGHT 0
#define EDITOR_KEY_LEFT 1
#define EDITOR_KEY_UP 2
#define EDITOR_KEY_DOWN 3

typedef struct Editor {
  QUILL_ELEMENT

  struct File *file;

  Rect file_rect;
  Cursor cursor;
  u32 col_offset;
  u32 line_offset;

  bool selected;
  Cursor selection_mark;

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
void editor_draw_lines(struct Painter *painter, Editor *editor, u32 start, u32 end, u32 editor_line_pos);
void editor_draw_text(struct Painter *painter, Editor *editor);


#endif /* _QUILL_EDITOR_H_ */
