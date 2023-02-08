#ifndef _QUILL_EDITOR_H_
#define _QUILL_EDITOR_H_

#include "quill.h"
#include "quill_element.h"
#include "quill_cursor.h"

struct Painter;
struct Line;

/* TODO: Get good keycodes for the editor keys */

#define EDITOR_MOD_MASK 0xf0000000
#define EDITOR_KEY_MASK 0x0fffffff

#define EDITOR_MOD_SHIFT_RIGHT 0x80000000
#define EDITOR_MOD_SHIFT_LEFT 0x40000000
#define EDITOR_MOD_SHIFT (EDITOR_MOD_SHIFT_RIGHT|EDITOR_MOD_SHIFT_LEFT)

#define EDITOR_MOD_CRTL_LEFT 0x20000000
#define EDITOR_MOD_CRTL_RIGHT 0x10000000
#define EDITOR_MOD_CRTL (EDITOR_MOD_CRTL_RIGHT|EDITOR_MOD_CRTL_LEFT)

#define EDITOR_MESSAGE EditorMessageType type;

/* TODO: Define this structures in quill_message.h */

typedef enum EditorMessageType {
  EDITOR_KEY_RIGHT,
  EDITOR_KEY_LEFT,
  EDITOR_KEY_UP,
  EDITOR_KEY_DOWN,
  EDITOR_KEY_RETURN,
  EDITOR_KEY_ENTER,
  EDITOR_KEY_DELETE,
  EDITOR_KEY_TAB,

  EDITOR_BUTTON_LEFT,

  EDITOR_KEY_P

} EditorMessageType;

typedef struct ButtonMessage {
  EDITOR_MESSAGE
  i32 x;
  i32 y;
} ButtonMessage;

typedef union EditorMessage {
  EDITOR_MESSAGE
  ButtonMessage button;
} EditorMessage;

#define EDITOR_DEFAULT_TAB_SIZE 2

typedef struct Editor {
  QUILL_ELEMENT

  u32 tab_size;

  struct File *file;

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

bool editor_should_scroll(Editor *editor);

bool editor_is_selected(Editor *editor, u32 line, u32 col);
void editor_draw_lines(struct Painter *painter, Editor *editor, u32 start, u32 end);
void editor_draw_cursor(struct Painter *painter, Editor *editor);
void editor_draw(struct Painter *painter, Editor *editor);
void editor_draw_text(struct Painter *painter, Editor *editor);

#endif /* _QUILL_EDITOR_H_ */
