#include "quill_editor.h"
#include "quill_line.h"
#include "quill_file.h"
#include "quill_painter.h"
#include "quill_data_structures.h"

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
  /* TODO: Look why descender is use only in cursor calculation */
  i32 t = editor_line_to_screen_pos(editor, editor_cursor_line_to_editor_visible_line(editor)) - platform.font->descender;
  i32 b = t + platform.font->line_gap;

  Rect rect = rect_create(l, r, t, b);
  return rect;
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

static u8 editor_get_current_codepoint(Editor *editor) {
  if(editor->cursor.col > 0) {
    return line_get_codepoint_at(file_get_line_at(editor->file, editor->cursor.line), (editor->cursor.col - 1));
  } else {
    return '\n';
  }
}

static inline void editor_undo_file_command_start(Editor *editor, u8 codepoint, FileCommandType type, bool sequence_enable) {
  File *file = editor->file;

  FileCommand *command = file_command_stack_top(file->undo_stack);

  bool is_sequence = false;
  if(sequence_enable) {
    is_sequence = (command &&
                        (command->type == type) &&
                        (command->end.line == editor->cursor.line) &&
                        (command->end.col == editor->cursor.col));

    is_sequence = is_sequence && (codepoint != ' ');
  }

  if(is_sequence) {
    vector_push(command->text, codepoint);
  } else {
    command = file_command_stack_push(file->undo_stack);
    command->type = type;
    vector_clear(command->text);
    vector_push(command->text, codepoint);
    command->start = editor->cursor;
  }
}

static inline void editor_undo_file_command_end(Editor *editor) {
  File *file = editor->file;
  FileCommand *command = file_command_stack_top(file->undo_stack);
  assert(command);
  command->end = editor->cursor;
}

static inline void editor_push_and_process_undo_command(Editor *editor) {
  File *file = editor->file;
  FileCommandStack *undo_stack = file->undo_stack;
  if(undo_stack->size > 0) {
    FileCommand *command = file_command_stack_pop(undo_stack);
    Cursor start = cursor_min(command->start, command->end);
    Cursor end = cursor_max(command->start, command->end);

    switch(command->type) {
    case FILE_COMMAND_REMOVE: {

      editor_remove_range(editor, start, end);

      printf("FILE_COMMAND_REMOVE\n");
      vector_push(command->text, '\0');
      printf("%s\n", command->text);
      cursor_print(start);
      cursor_print(end);

    } break;
    case FILE_COMMAND_INSERT: {

      editor_add_range(editor, command->text, start, end);

      printf("FILE_COMMAND_INSERT\n");
      vector_push(command->text, '\0');
      printf("%s\n", command->text);
      cursor_print(start);
      cursor_print(end);

    } break;
    case FILE_COMMAND_JOIN_LINES: {

      printf("FILE_COMMAND_JOIN_LINES\n");
      vector_push(command->text, '\0');
      printf("%s\n", command->text);
      cursor_print(start);
      cursor_print(end);

    } break;
    case FILE_COMMAND_SPLIT_LINE: {

      printf("FILE_COMMAND_SPLIT_LINE\n");
      vector_push(command->text, '\0');
      printf("%s\n", command->text);
      cursor_print(start);
      cursor_print(end);

    } break;
    default: {} break;
    }
  }
}

static int editor_default_message_handler(struct Element *element, Message message, void *data) {
  Editor *editor = (Editor *)element;

  switch(message) {
  case MESSAGE_DRAW: {
    Painter *painter = (Painter *)data;
    editor_draw(painter, editor);
  } break;
  case MESSAGE_DRAW_ON_TOP: {

  } break;
  case MESSAGE_RESIZE: {

  } break;
  case MESSAGE_KEYDOWN: {
    /* TODO: Find a good way to handle when the editor has no file */
    if(editor->file) {
      u32 mod = (u32)((u64)data) & EDITOR_MOD_MASK;
      u32 key = (u32)((u64)data) & EDITOR_KEY_MASK;

      switch(key) {
      case EDITOR_KEY_LEFT: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        if(EDITOR_MOD_IS_SET(mod, EDITOR_MOD_CRTL)) {
          editor_step_next_token_left(editor);
        } else {
          editor_step_cursor_left(editor);
        }
      } break;
      case EDITOR_KEY_RIGHT: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        if(EDITOR_MOD_IS_SET(mod, EDITOR_MOD_CRTL)) {
          editor_step_next_token_right(editor);
        } else {
          editor_step_cursor_right(editor);
        }
      } break;
      case EDITOR_KEY_UP: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_up(editor);
      } break;
      case EDITOR_KEY_DOWN: {
        /* TODO: BUG: When scrolling down selection disappears */
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_down(editor);
      } break;
      case EDITOR_KEY_HOME: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_start(editor);
      } break;
      case EDITOR_KEY_END: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_end(editor);
      } break;
      case EDITOR_KEY_PAGE_DOWN: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_page_down(editor);
      } break;
      case EDITOR_KEY_PAGE_UP: {
        editor_update_selected(editor, EDITOR_MOD_IS_SET(mod, EDITOR_MOD_SHIFT));
        editor_step_cursor_page_up(editor);
      } break;
      case EDITOR_KEY_RETURN: {
        u8 codepoint = editor_get_current_codepoint(editor);
        editor_undo_file_command_start(editor, codepoint, FILE_COMMAND_INSERT, false);
        editor_cursor_remove(editor);
        editor_undo_file_command_end(editor);
      } break;
      case EDITOR_KEY_DELETE: {
        editor_cursor_remove_right(editor);
      } break;
      case EDITOR_KEY_ENTER: {
        editor_cursor_insert_new_line(editor);
      } break;
      case EDITOR_KEY_TAB: {
        for(u32 i = 0; i < editor->tab_size; ++i) {
          editor_cursor_insert(editor, ' ');
        }
      } break;
      case EDITOR_KEY_C: {
        if(EDITOR_MOD_IS_SET(mod, EDITOR_MOD_CRTL)) {
          editor_copy_selection_to_clipboard(editor);
        }
      } break;
      case EDITOR_KEY_V: {
        if(EDITOR_MOD_IS_SET(mod, EDITOR_MOD_CRTL)) {
          editor_paste_clipboard(editor);
        }
      } break;
      case EDITOR_KEY_Z: {
        if(EDITOR_MOD_IS_SET(mod, EDITOR_MOD_CRTL)) {
          editor_push_and_process_undo_command(editor);
        }
      } break;

      }
      element_update(editor);
    }
  } break;
  case MESSAGE_KEYUP: {

  } break;
  case MESSAGE_TEXTINPUT: {
    /* TODO: Find a good way to handle when the editor has no file */
    if(editor->file) {
      u8 codepoint = (u8)(u64)data;
      editor_undo_file_command_start(editor, codepoint, FILE_COMMAND_REMOVE, true);
      editor_cursor_insert(editor, codepoint);
      editor_undo_file_command_end(editor);

      element_update(editor);
    }
  } break;
  case MESSAGE_BUTTONDOWN: {
  } break;
  case MESSAGE_EDITOR_OPEN_FILE: {
    File *file = (File *)data;
    editor->file = file;
    editor->cursor = file->cursor_saved;
  } break;
  default: {} break;
  }

  return 0;
}

static void editor_user_element_destroy(Element *element) {
  (void)element;
  printf("Editor destroy\n");
}

Editor *editor_create(Element *parent) {
  Editor *editor = (Editor *)element_create(sizeof(Editor), parent, editor_default_message_handler);
  element_set_user_element_destroy(&editor->element, editor_user_element_destroy);
  editor->tab_size = EDITOR_DEFAULT_TAB_SIZE;
  return editor;
}

bool editor_should_scroll(Editor *editor) {
  Cursor *cursor = &editor->cursor;

  u32 total_codepoints_view = element_get_width(editor) / platform.font->advance;
  u32 total_lines_view = element_get_height(editor) / platform.font->line_gap;

  u32 old_col_offset = editor->col_offset;
  if(total_codepoints_view) {
    if(cursor->col < editor->col_offset) {
      editor->col_offset = cursor->col;
    } else if(cursor->col > (editor->col_offset + (total_codepoints_view - 1))) {
      editor->col_offset = cursor->col - (total_codepoints_view - 1);
    }
  }
  u32 old_line_offset = editor->line_offset;
  if(total_lines_view) {
    if(cursor->line < editor->line_offset) {
      editor->line_offset = cursor->line;
    } else if(cursor->line > (editor->line_offset + (total_lines_view - 1))) {
      editor->line_offset = cursor->line - (total_lines_view - 1);
    }
  }

  bool scroll = (old_line_offset != editor->line_offset) || (old_col_offset != editor->col_offset);
  return scroll;
}

void editor_step_cursor_left(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->col > 0) {
    --cursor->col;
      cursor->save_col = cursor->col;
  } else if(cursor->line > 0) {
    cursor->col = line_size(file_get_line_at(file, cursor->line - 1));
    cursor->save_col = cursor->col;
    --cursor->line;
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
  }

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

void editor_step_cursor_right(Editor *editor) {
  File *file = editor->file; (void)file;
  Cursor *cursor = &editor->cursor;
  Line *line = file_get_line_at(file, cursor->line);

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->col < line_size(line)) {
    ++cursor->col;
    cursor->save_col = cursor->col;
  } else if(cursor->line < (file_line_count(file) - 1)) {
    cursor->col = 0;
    cursor->save_col = cursor->col;
    ++cursor->line;
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
  }

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

void editor_step_cursor_up(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line <= file_line_count(file));

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->line > 0) {
    --cursor->line;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
  }

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

void editor_step_cursor_down(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));

  Rect rect = editor_get_cursor_line_rect(editor);

  if(cursor->line < (file_line_count(file) - 1)) {
    ++cursor->line;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
    rect = rect_union(rect, editor_get_cursor_line_rect(editor));
  }

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

float page_percentage = 0.7f;

void editor_step_cursor_page_down(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  u32 step = MIN((u32)((float)editor_max_visible_lines(editor)*page_percentage), (file_line_count(file) - cursor->line));
  for(u32 i = 0; i < step; ++i) {
    editor_step_cursor_down(editor);
  }
}

void editor_step_cursor_page_up(Editor *editor) {
  Cursor *cursor = &editor->cursor;
  u32 step = MIN((u32)((float)editor_max_visible_lines(editor)*page_percentage), cursor->line);
  for(u32 i = 0; i < step; ++i) {
    editor_step_cursor_up(editor);
  }
}

void editor_step_cursor_start(Editor *editor) {
  Cursor *cursor = &editor->cursor;

  Rect rect = editor_get_cursor_line_rect(editor);

  cursor->col = 0;
  cursor->save_col = cursor->col;

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

void editor_step_cursor_end(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;

  Rect rect = editor_get_cursor_line_rect(editor);

  cursor->col = line_size(file_get_line_at(file, cursor->line));
  cursor->save_col = cursor->col;

  bool scroll = editor_should_scroll(editor);
  element_redraw(editor, scroll ? &element_get_rect(editor) : &rect);
}

static inline bool codepoint_is_separator(u8 codepoint) {
  return (codepoint == ',') || (codepoint == ';') || (codepoint == '.') ||
      (codepoint == '-') || (codepoint == '_') || (codepoint == '>') ||
      (codepoint == '(') || (codepoint == ')') || (codepoint == ' ') ||
      (codepoint == '/') || (codepoint == '\\');
}

void editor_step_next_token_left(Editor *editor) {
  Line *line = file_get_line_at(editor->file, editor->cursor.line);
  if(editor->cursor.col > 0) {

    if(editor->cursor.col == line_size(line)) {
      editor_step_cursor_left(editor);
    }

    u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);
    u8 left_codepoint = line_get_codepoint_at(line, editor->cursor.col - 1);
    if(codepoint_is_separator(left_codepoint)) {
      editor_step_cursor_left(editor);
      codepoint = line_get_codepoint_at(line, editor->cursor.col);
    }

    if(codepoint == ' ') {
      while(editor->cursor.col > 0) {
        editor_step_cursor_left(editor);

        if(editor->cursor.col == 0) {
          return;
        }

        u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);
        if(codepoint != ' ') {
          break;
        }
      }
    }

    for(;;) {
      editor_step_cursor_left(editor);
      if(editor->cursor.col == 0) {
        return;
      }

      u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);

      if(codepoint_is_separator(codepoint)) {
        editor_step_cursor_right(editor);
        return;
      }
    }

  } else {
    editor_step_cursor_left(editor);
  }
}

void editor_step_next_token_right(Editor *editor) {
  Line *line = file_get_line_at(editor->file, editor->cursor.line);
  if(editor->cursor.col < (line_size(line))) {

    u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);
    if(codepoint == ' ') {
      while(editor->cursor.col < line_size(line)) {
        editor_step_cursor_right(editor);

        if(editor->cursor.col == line_size(line)) {
          return;
        }

        u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);
        if(codepoint != ' ') {
          break;
        }
      }
    }

    for(;;) {
      editor_step_cursor_right(editor);
      if(editor->cursor.col == line_size(line)) {
        return;
      }

      u8 codepoint = line_get_codepoint_at(line, editor->cursor.col);

      if(codepoint_is_separator(codepoint)) {
        return;
      }
    }

  } else {
    editor_step_cursor_right(editor);
  }
}

void editor_cursor_insert(Editor *editor, u8 codepoint) {
  if(editor->selected) {
    editor_remove_selection(editor);
  }
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
  Line *old_line = file_get_line_at(file, cursor->line + 1);

  line_copy(new_line, old_line, cursor->col);
  line_remove_from_front_up_to(old_line, cursor->col);

  cursor->save_col = 0;
  editor_step_cursor_down(editor);

  element_redraw(editor, 0);

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

      cursor->save_col = second_line_size;
      editor_step_cursor_up(editor);
    }
  } else {
    line_remove_at_index(line, cursor->col);
    editor_step_cursor_left(editor);
  }

  element_redraw(editor, 0);
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

  element_redraw(editor, 0);
}

/* TODO: Move this function into line API */
static inline void line_remove_range(Line *line, u32 start, u32 end) {
  u32 remove_cout = end - start;
  for(u32 i = 0; i < remove_cout; ++i) {
    line_remove_at_index(line, start + 1);
  }
}

void editor_paste_clipboard(Editor *editor) {
  u8 *clipboard = platform_get_clipboard();
  u8 *iterator = clipboard;
  for(;;) {
    u8 codepoint = *iterator++;
    if(codepoint == '\0') {
      break;
    }
    if(codepoint == '\n') {
      editor_cursor_insert_new_line(editor);
      continue;
    }
    editor_cursor_insert(editor, codepoint);
  }
  platform_free_clipboard(clipboard);
}

void editor_copy_selection_to_clipboard(Editor *editor) {
  if(editor->selected) {
    File *file = editor->file;
    Cursor *cursor = &editor->cursor;
    platform_temp_clipboard_clear(&platform);

    Cursor start = cursor_min(editor->cursor, editor->selection_mark);
    Cursor end = cursor_max(editor->cursor, editor->selection_mark);

    for(u32 i = start.line; i <= end.line; ++i) {
      Line *line = file_get_line_at(file, i);
      u32 size = line_size(line);
      if(start.line == end.line) {
        for(u32 j = start.col; j < end.col; ++j) {
          platform_temp_clipboard_push(&platform, line_get_codepoint_at(line, j));
        }
      } else if(i == start.line) {
        for(u32 j = start.col; j < size; ++j) {
          platform_temp_clipboard_push(&platform, line_get_codepoint_at(line, j));
        }
      } else if(i == end.line) {
        for(u32 j = 0; j < end.col; ++j) {
          platform_temp_clipboard_push(&platform, line_get_codepoint_at(line, j));
        }
      } else {
        for(u32 j = 0; j < size; ++j) {
          platform_temp_clipboard_push(&platform, line_get_codepoint_at(line, j));
        }
      }
      if(i < end.line) {
        platform_temp_clipboard_push(&platform, '\n');
      }
    }
    platform_temp_clipboard_push(&platform, '\0');
    platform_set_clipboard(platform.temp_clipboard);

    editor->selected = false;
    *cursor = start;

    element_redraw(editor, 0);

  }
}

void editor_add_range(Editor *editor, u8 *text, Cursor start, Cursor end) {
  editor->cursor = start;
  u32 text_size = vector_size(text);
  for(u32 i = 0; i < text_size; ++i) {
    u8 codepoint = text[i];
    if(codepoint == '\n') {
      editor_cursor_insert_new_line(editor);
    } else {
      editor_cursor_insert(editor, codepoint);
    }
  }

  printf("editor cursor:\n");
  cursor_print(editor->cursor);
  printf("end cursor:\n");
  cursor_print(end);

  assert((editor->cursor.line == end.line) &&
         (editor->cursor.col == end.col));

  editor_should_scroll(editor);
  element_redraw(editor, 0);
}


void editor_remove_range(Editor *editor, Cursor start, Cursor end) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
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

  editor_should_scroll(editor);
  element_redraw(editor, 0);
}

void editor_remove_selection(Editor *editor) {
  Cursor start = cursor_min(editor->cursor, editor->selection_mark);
  Cursor end = cursor_max(editor->cursor, editor->selection_mark);
  editor_remove_range(editor, start, end);
}

void editor_update_selected(Editor *editor, bool selected) {
  if(!editor->selected && selected) {
    editor->selected = true;
    editor->selection_mark = editor->cursor;
  } else if(!selected) {

    if(editor->selected) {
      element_redraw(editor, 0);
    }

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
  if(!file) {
    return;
  }
  for(u32 i = start; i < end; ++i) {

    u32 screen_x = element_get_rect(editor).l - editor->col_offset * platform.font->advance;
    u32 screen_y = editor_line_to_screen_pos(editor, i) + platform.font->line_gap;

    /* TODO: Handle draw end of file outside this loop */
    u32 line_index = i + editor->line_offset;
    if(line_index >= file_line_count(file)) {
      return;
    }
    Line *line = file_get_line_at(file, line_index);

    painter_draw_line(painter, line, screen_x, screen_y, 0xd0d0d0);
  }
}

void editor_draw_cursor(struct Painter *painter, Editor *editor) {
  i32 line = editor_line_to_screen_pos(editor, editor_cursor_line_to_editor_visible_line(editor));
  i32 col = editor_col_to_screen_pos(editor, editor_cursor_col_to_editor_visible_col(editor));
  i32 l = col;
  i32 r = l + 2;
  /* TODO: Look why descender is use only in cursor calculation */
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

static void editor_draw_selection(Painter *painter, Editor *editor, u32 start, u32 end) {
  File *file = editor->file;
  if(!file) {
    return;
  }

  Cursor start_selection = cursor_min(editor->cursor, editor->selection_mark);
  Cursor end_selection = cursor_max(editor->cursor, editor->selection_mark);

  start = MAX(start, start_selection.line - editor->line_offset);
  end = MIN(end, end_selection.line - editor->line_offset);

  for(u32 i = start; i <= end; ++i) {

    /* TODO: Handle draw end of file outside this loop */
    u32 line_index = i + editor->line_offset;
    if(line_index >= file_line_count(file)) {
      return;
    }

    u32 screen_x = element_get_rect(editor).l - editor->col_offset * platform.font->advance;
    u32 screen_y = editor_line_to_screen_pos(editor, i) - platform.font->descender;

    Line *line = file_get_line_at(file, line_index);
    u32 size = line_size(line);
    Rect line_rect = rect_create(screen_x, screen_x + MAX(size * platform.font->advance, platform.font->advance/2),
                                 screen_y, screen_y + platform.font->line_gap);

    if((line_index == start_selection.line) && (line_index == end_selection.line)) {
      line_rect.l += start_selection.col * platform.font->advance;
      line_rect.r -= (size - end_selection.col) * platform.font->advance;
    } else if(line_index == start_selection.line) {
      line_rect.l += start_selection.col * platform.font->advance;
    } else if(line_index == end_selection.line) {
      line_rect.r -= (size - end_selection.col) * platform.font->advance;
    }

    painter_draw_rect(painter, line_rect, 0x0000ff);
  }
}

void editor_draw(struct Painter *painter, Editor *editor) {
  Range lines = editor_rect_instersect_lines(editor, painter->clipping);

  /* TODO: Refactor the rendering code this is really a hack to get it running */

  if(range_is_valid(lines)) {
    Rect rect = rect_intersection(painter->clipping, element_get_rect(editor));
    painter_draw_rect(painter, rect, 0x202020);
    if(editor->selected) {
      editor_draw_selection(painter, editor, lines.start, lines.end);
    }
    editor_draw_lines(painter, editor, lines.start, lines.end);
    editor_draw_cursor(painter, editor);
  }
}

#if 0
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
#endif
