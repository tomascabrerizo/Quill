#include "quill_editor.h"
#include "quill_line.h"
#include "quill_file.h"
#include "quill_painter.h"

extern Platform platform;

static inline u32 editor_line_to_screen_pos(Editor *editor, u32 line_pos) {
  u32 line_height = platform.font->line_gap;
  return element_get_rect(editor).t + (line_pos * line_height);
}

static inline u32 editor_col_to_screen_pos(Editor *editor, u32 col_pos) {
  u32 advance = platform.font->advance;
  return element_get_rect(editor).l + (col_pos * advance);
}

static inline u32 editor_screen_to_line_pos(Editor *editor, u32 screen_pos) {
  u32 line_height = platform.font->line_gap;
  return (screen_pos - element_get_rect(editor).t) / line_height;
}

static inline u32 editor_max_visible_lines(Editor *editor) {
  u32 line_height = platform.font->line_gap;
  return element_get_height(editor) / line_height;
}

static inline u32 editor_cursor_line_to_editor_visible_line(Editor *editor) {
  return editor->cursor.line - editor->line_offset;
}

static inline u32 editor_cursor_col_to_editor_visible_col(Editor *editor) {
  return editor->cursor.col - editor->col_offset;
}

static inline Rect editor_get_cursor_line_rect(Editor *editor) {
  i32 l = element_get_rect(editor).l;
  i32 r = element_get_rect(editor).r;

  i32 t = editor_line_to_screen_pos(editor, editor_cursor_line_to_editor_visible_line(editor)) - platform.font->descender;
  i32 b = t + platform.font->line_gap;

  Rect rect = rect_create(l, r, t, b);
  return rect;
}

void cursor_print(Cursor cursor) {
  printf("line:%d, col:%d\n", cursor.line, cursor.col);
}


static int editor_default_message_handler(struct Element *element, Message message, void *data) {
  Editor *editor = (Editor *)element;

  switch(message) {
  case MESSAGE_DRAW: {
    Painter *painter = (Painter *)data;
    editor_draw(painter, editor);
  } break;
  case MESSAGE_RESIZE: {

  } break;
  case MESSAGE_KEYDOWN: {
    u32 key = (u32)((u64)data);
    switch(key) {
    case EDITOR_KEY_LEFT: {
      Rect rect = editor_step_cursor_left(editor);
      element_redraw(editor, &rect);
    } break;
    case EDITOR_KEY_RIGHT: {
      Rect rect = editor_step_cursor_right(editor);
      element_redraw(editor, &rect);
    } break;
    case EDITOR_KEY_UP: {
      Rect rect = editor_step_cursor_up(editor);
      element_redraw(editor, &rect);
    } break;
    case EDITOR_KEY_DOWN: {
      Rect rect = editor_step_cursor_down(editor);
      element_redraw(editor, &rect);
    } break;
    }

    element_update(editor);

  } break;
  case MESSAGE_KEYUP: {

  } break;
  }

  return 0;
}

static void editor_user_element_destroy(Element *element) {
  Editor *editor = (Editor *)element;
  if(editor->file) {
    file_destroy(editor->file);
  }
}

Editor *editor_create(Element *parent) {
  Editor *editor = (Editor *)element_create(sizeof(Editor), parent, editor_default_message_handler);
  element_set_user_element_destroy(&editor->element, editor_user_element_destroy);
  return editor;
}


Rect editor_step_cursor_right(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line <= file_line_count(file));
  Line *line = file_get_line_at(file, cursor->line);

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->col < line_size(line)) {
    u32 total_codepoints_view = element_get_width(editor) / platform.font->advance;
    if(line_size(line) > total_codepoints_view) {
      u32 one_pass_last_view_codepoint = MIN(editor->col_offset + total_codepoints_view, line_size(line));
      if(cursor->col == (one_pass_last_view_codepoint - 1)) {
        editor->col_offset++;
      }
    }
    cursor->col++;
  } else if(cursor->line < (file_line_count(file) - 1)) {
    rect = rect_union(rect, editor_step_cursor_down(editor));
    cursor->col = 0;
    editor->col_offset = 0;
  }
  cursor->save_col = cursor->col;

  return rect;
}

Rect editor_step_cursor_left(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->col > 0) {
    if(cursor->col == editor->col_offset) {
      editor->col_offset--;
    }

    cursor->col--;
  } else if(cursor->line > 0) {
    rect = rect_union(rect, editor_step_cursor_up(editor));
    cursor->col = line_size(file_get_line_at(file, cursor->line));
  }
  cursor->save_col = cursor->col;

  return rect;
}

Rect editor_step_cursor_up(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line <= file_line_count(file));

  if(cursor->line > 0) {
    bool scroll = false;

    if(cursor->line == editor->line_offset) {
      editor->line_offset--;
      scroll = true;
    }

    Rect rect = editor_get_cursor_line_rect(editor);
    cursor->line--;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
    return scroll ? element_get_rect(editor) : rect;
  }
  return rect_create(0, 0, 0, 0);
}

Rect editor_step_cursor_down(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));


  if(cursor->line < (file_line_count(file) - 1)) {
    bool scroll = false;

    u32 total_lines_view = element_get_height(editor) / platform.font->line_gap;
    u32 one_pass_last_view_line = MIN(editor->line_offset + total_lines_view, file_line_count(file));
    if(cursor->line == (one_pass_last_view_line - 1)) {
      editor->line_offset++;
      scroll = true;
    }

    Rect rect = editor_get_cursor_line_rect(editor);
    cursor->line++;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
    return scroll ? element_get_rect(editor) : rect;
  }
  return rect_create(0, 0, 0, 0);
}

void editor_cursor_insert(Editor *editor, u8 codepoint) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));
  Line *line = file_get_line_at(file, cursor->line);
  line_insert_at_index(line, cursor->col, codepoint);
  editor_step_cursor_right(editor);
}

void editor_cursor_insert_new_line(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));
  file_insert_new_line_at(file, cursor->line);

  Line *new_line = file_get_line_at(file, cursor->line);
  editor_step_cursor_down(editor);
  Line *old_line = file_get_line_at(file, cursor->line);

  line_copy(new_line, old_line, cursor->col);
  line_remove_from_front_up_to(old_line, cursor->col);

  cursor->col = 0;
  cursor->save_col = cursor->col;
}

void editor_cursor_remove(Editor *editor) {
  if(editor->selected) {
    editor_remove_selection(editor);
    return;
  }

  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));
  Line *line = file_get_line_at(file, cursor->line);
  if(cursor->col == 0) {
    if(cursor->line > 0) {
      Line *first_line = file_get_line_at(file, cursor->line);
      Line *second_line = file_get_line_at(file, cursor->line - 1);
      u32 second_line_size = line_size(second_line);
      line_copy_at(first_line, second_line, second_line_size, 0);
      file_remove_line_at(file, cursor->line);
      editor_step_cursor_up(editor);
      cursor->col = second_line_size;
      cursor->save_col = cursor->col;
    }
  } else {
    line_remove_at_index(line, cursor->col);
    editor_step_cursor_left(editor);
  }
}

void editor_cursor_remove_right(Editor *editor) {
  if(editor->selected) {
    editor_remove_selection(editor);
    return;
  }

  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));
  Line *line = file_get_line_at(file, cursor->line);
  if(cursor->col == line_size(line)) {
    if(cursor->line < (file_line_count(file) - 1)) {
      Line *first_line = file_get_line_at(file, cursor->line);
      Line *second_line = file_get_line_at(file, cursor->line + 1);
      u32 first_line_size = line_size(first_line);
      line_copy_at(second_line, first_line, first_line_size, 0);
      file_remove_line_at(file, cursor->line + 1);
    }
  } else {
    line_remove_at_index(line, cursor->col + 1);
  }
}

static inline Cursor cursor_min(Cursor a, Cursor b) {
  if(a.line == b.line) {
    return (a.col < b.col) ? a : b;
  }
  return (a.line < b.line) ? a : b;
}

static inline Cursor cursor_max(Cursor a, Cursor b) {
  if(a.line == b.line) {
    return (a.col > b.col) ? a : b;
  }
  return (a.line > b.line) ? a : b;
}

/* TODO: Move this function into line API */
static inline void line_remove_range(Line *line, u32 start, u32 end) {
  u32 remove_cout = end - start;
  for(u32 i = 0; i < remove_cout; ++i) {
    line_remove_at_index(line, start + 1);
  }
}

void editor_remove_selection(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;

  Cursor start = cursor_min(editor->cursor, editor->selection_mark);
  Cursor end = cursor_max(editor->cursor, editor->selection_mark);

  if(start.line == end.line) {
    Line *line = file_get_line_at(file, start.line);
    line_remove_range(line, start.col, end.col);
  } else if((end.line - start.line) > 0) {

    Line *line_start = file_get_line_at(file, start.line);
    line_remove_range(line_start, start.col, line_size(line_start));
    Line *line_end = file_get_line_at(file, end.line);
    line_remove_range(line_end, 0, end.col);
    line_copy_at(line_start, line_end, line_size(line_end), line_size(line_start));
    file_remove_line_at(file, end.line + 1);

    u32 middle_lines_count = end.line - start.line - 1;
    for(u32 i = 0; i < middle_lines_count; ++i) {
      file_remove_line_at(file, start.line + 2);
    }
  }

  editor->selected = false;
  *cursor = start;
  return;
}

void editor_update_selected(Editor *editor, bool selected) {
  if(!editor->selected && selected) {
    editor->selected = true;
    editor->selection_mark = editor->cursor;
  } else if(!selected) {
    editor->selected = false;
    editor->selection_mark = editor->cursor;
  }
}

bool editor_is_selected(Editor *editor, u32 line, u32 col) {
  if(editor->selected) {

    Cursor start = cursor_min(editor->cursor, editor->selection_mark);
    Cursor end = cursor_max(editor->cursor, editor->selection_mark);

    if(start.line == end.line && start.line == line) {
      if(col >= start.col && col < end.col) {
        return true;
      }
    } else if(line == start.line && line <= end.line) {
      if(col >= start.col) {
        return true;
      }
    } else if(line == end.line && line >= start.line) {
      if(col < end.col) {
        return true;
      }
    } else if((line > start.line) && (line < end.line)) {
      return true;
    }
    return false;
  }
  return false;
}

void editor_draw_lines(Painter *painter, Editor *editor, u32 start, u32 end) {
  File *file = editor->file;
  for(u32 i = start; i < end; ++i) {
    u32 screen_x = element_get_rect(editor).l;
    u32 screen_y = editor_line_to_screen_pos(editor, i) + platform.font->line_gap;
    Line *line = file_get_line_at(file, i + editor->line_offset);
    painter_draw_line(painter, line, screen_x, screen_y, 0xd0d0d0);
  }
}

void editor_draw_cursor(struct Painter *painter, Editor *editor) {
  i32 line = editor_line_to_screen_pos(editor, editor_cursor_line_to_editor_visible_line(editor));
  i32 col = editor_col_to_screen_pos(editor, editor_cursor_col_to_editor_visible_col(editor));
  i32 l = col;
  i32 r = l + 2;
  i32 t = line - painter->font->descender;
  i32 b = t + platform.font->line_gap;
  Rect cursor_rect = rect_create(l, r, t, b);
  painter_draw_rect(painter, cursor_rect, 0xff00ff);
}

typedef struct Range {
  u32 start;
  u32 end;
} Range;

static inline bool range_is_valid(Range range) {
  return range.start < range.end;
}

static inline Range range_invalid() {
  Range range;
  range.end = 0;
  range.start = 1;
  return range;
}

static inline void range_print(Range range) {
  printf("start: %d, end: %d\n", range.start, range.end);
}

static inline Range editor_rect_instersect_lines(Editor *editor, Rect rect) {
  rect = rect_intersection(rect, element_get_rect(editor));
  if(rect_is_valid(rect)) {
    Range range;
    range.start = editor_screen_to_line_pos(editor, rect.t);
    range.end = editor_screen_to_line_pos(editor, rect.b);
    return range;
  }
  return range_invalid();
}

void editor_draw(struct Painter *painter, Editor *editor) {
  Range lines = editor_rect_instersect_lines(editor, painter->clipping);
  range_print(lines);
  if(range_is_valid(lines)) {
    Rect rect = rect_intersection(painter->clipping, element_get_rect(editor));
    painter_draw_rect(painter, rect, 0x202020);
    editor_draw_lines(painter, editor, lines.start, lines.end);
    editor_draw_cursor(painter, editor);
  }
}


void editor_draw_text(Painter *painter, Editor *editor) {

  painter_draw_rect(painter, editor->element.rect, 0x202020);

  File *file = editor->file;
  assert(file);

  i32 start_x = editor->element.rect.l;
  i32 start_y = editor->element.rect.t;

  u32 total_lines_to_render = (element_get_height(editor) - start_y) / platform.font->line_gap;
  u32 max_lines_to_render = MIN(editor->line_offset + total_lines_to_render, file_line_count(file));

  i32 pen_x = start_x;
  i32 pen_y = start_y + painter->font->line_gap;
  for(u32 i = editor->line_offset; i < max_lines_to_render; ++i) {
    Line *line = file_get_line_at(file, i);
    for(u32 j = editor->col_offset; j < line_size(line); ++j) {

      u8 codepoint = line_get_codepoint_at(line, j);
      Glyph *glyph = &painter->font->glyph_table[codepoint];

      if(editor_is_selected(editor, i, j)) {
        i32 x = pen_x;
        i32 y = pen_y - painter->font->line_gap - painter->font->descender;
        Rect rect = rect_create(x, x + painter->font->advance,
                                y, y + painter->font->line_gap);
        painter_draw_rect(painter, rect, 0x2761fa);
      }

      painter_draw_glyph(painter, glyph, pen_x, pen_y, 0xd0d0d0);
      pen_x += painter->font->advance;
    }
    pen_x = start_x;
    pen_y += painter->font->line_gap;
  }

  Cursor cursor = editor->cursor;
  i32 l = start_x + ((i32)cursor.col - (i32)editor->col_offset) * painter->font->advance;
  i32 r = l + 2;
  i32 t = start_y + ((i32)cursor.line - (i32)editor->line_offset) * painter->font->line_gap - painter->font->descender;
  i32 b = t + painter->font->line_gap;
  Rect cursor_rect = rect_create(l, r, t, b);
  painter_draw_rect(painter, cursor_rect, 0xff00ff);
}
