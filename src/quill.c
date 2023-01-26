#include "quill.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

Platform platform;

Rect rect_create(i32 l, i32 r, i32 t, i32 b) {
  Rect rect = {l, r, t, b};
  return rect;
}

bool rect_is_valid(Rect rect) {
  return rect.r > rect.l && rect.b > rect.t;
}

Rect rect_intersection(Rect a, Rect b) {
  Rect result;
  result.l = MAX(a.l, b.l);
  result.r = MIN(a.r, b.r);
  result.t = MAX(a.t, b.t);
  result.b = MIN(a.b, b.b);
  return result;
}

Rect rect_union(Rect a, Rect b) {
  Rect result;
  result.l = MIN(a.l, b.l);
  result.r = MAX(a.r, b.r);
  result.t = MIN(a.t, b.t);
  result.b = MAX(a.b, b.b);
  return result;
}

void rect_print(Rect rect) {
  printf("l:%d, r:%d, t:%d, b:%d\n", rect.l, rect.r, rect.t, rect.b);
}


BackBuffer *backbuffer_create(i32 w, i32 h, u32 bytes_per_pixel) {
  BackBuffer *backbuffer = (BackBuffer *)malloc(sizeof(BackBuffer));
  backbuffer->w = w;
  backbuffer->h = h;
  backbuffer->bytes_per_pixel = bytes_per_pixel;
  backbuffer->update_region = rect_create(0, w, 0, h);
  backbuffer->pixels = (u32 *)malloc(w * h * bytes_per_pixel);
  return backbuffer;
}

void backbuffer_destroy(BackBuffer *backbuffer) {
  free(backbuffer->pixels);
  free(backbuffer);
}

void backbuffer_resize(BackBuffer *backbuffer, i32 w, i32 h) {
  backbuffer->w = w;
  backbuffer->h = h;
  backbuffer->update_region = rect_create(0, w, 0, h);
  backbuffer->pixels = (u32 *)realloc(backbuffer->pixels, w * h * backbuffer->bytes_per_pixel);
}

Painter painter_create(BackBuffer *backbuffer) {
  Painter painter;
  painter.clipping = backbuffer->update_region;
  painter.pixels = backbuffer->pixels;
  painter.w = backbuffer->w;
  painter.h = backbuffer->h;
  painter.font = 0;
  return painter;
}

void painter_set_font(Painter *painter, Font *font) {
  painter->font = font;
}


void painter_draw_rect(Painter *painter, Rect rect, u32 color) {
  rect = rect_intersection(rect, painter->clipping);
  for(i32 yy = rect.t; yy < rect.b; ++yy) {
    for(i32 xx = rect.l; xx < rect.r; ++xx) {
      painter->pixels[yy*painter->w+xx] = color;
    }
  }
}

void painter_draw_glyph(Painter *painter, Glyph *glyph, i32 x, i32 y, u32 color) {
  x += glyph->bearing_x;
  y -= glyph->bearing_y;

  Rect rect = rect_create(x, x + glyph->w, y, y + glyph->h);
  rect = rect_intersection(rect, painter->clipping);
  u32 *pixels_row = painter->pixels + rect.t * painter->w + rect.l;
  u8 *bytes_row = glyph->pixels + (rect.t - y) * glyph->w + (rect.l - x);
  for(i32 yy = rect.t; yy < rect.b; ++yy) {
    u32 *pixels = pixels_row;
    u8 *bytes = bytes_row;
    for(i32 xx = rect.l; xx < rect.r; ++xx) {
      u8 dr = (*pixels >> 16);
      u8 dg = (*pixels >>  8);
      u8 db = (*pixels >>  0);
      u8 sr = (color >> 16);
      u8 sg = (color >>  8);
      u8 sb = (color >>  0);
      float a = (float)*bytes++ / 255.0f;
      u8 r = (u8)(dr * (1.0f - a) + sr * a);
      u8 g = (u8)(dg * (1.0f - a) + sg * a);
      u8 b = (u8)(db * (1.0f - a) + sb * a);
      *pixels++ = (255 << 24) | (r << 16) | (g << 8) | (b << 0);
    }
    pixels_row += painter->w;
    bytes_row += glyph->w;
  }
}

void painter_draw_text(Painter *painter, u8 *text, u32 size, i32 x, i32 y, u32 color) {
  assert(painter->font);
  i32 pen_y = y; //(y + painter->font->line_gap);
  i32 pen_x = x;
  for(u32 i = 0; i < size; ++i) {
    u16 codepoint = (u16)text[i];
    Glyph *glyph = &painter->font->glyph_table[codepoint];
    if(codepoint != (u16)' ') {
      painter_draw_glyph(painter, glyph, pen_x, pen_y, color);
    }
    pen_x += painter->font->advance;
  }
}

void *gapbuffer_grow(void *buffer, u32 element_size) {
  if(buffer == 0) {
    GapBufferHeader *header = malloc(sizeof(GapBufferHeader) + element_size * GAPBUFFER_DEFAULT_CAPACITY);
    header->capacity = GAPBUFFER_DEFAULT_CAPACITY;
    header->f_index = 0;
    header->s_index = GAPBUFFER_DEFAULT_CAPACITY;
    return (void *)(header + 1);
  } else {
    GapBufferHeader *header = gapbuffer_header(buffer);
    u32 new_buffer_size = header->capacity * 2;
    header = (GapBufferHeader *)realloc(header, sizeof(GapBufferHeader) + new_buffer_size * element_size);
    u32 second_gap_size = header->capacity - header->s_index;
    void *des = (u8 *)(header + 1) + (new_buffer_size - second_gap_size) * element_size;
    void *src = (u8 *)(header + 1) + (header->capacity - second_gap_size) * element_size;
    memmove(des, src, second_gap_size * element_size);
    header->s_index = new_buffer_size - second_gap_size;
    header->capacity = new_buffer_size;
    return (header + 1);
  }
}

Line *line_create(void) {
  Line *line = (Line *)malloc(sizeof(Line));
  memset(line , 0, sizeof(Line));
  return line;
}

void line_destroy(Line *line) {
  assert(line);
  gapbuffer_free(line->buffer);
  free(line);
}

void line_reset(Line *line) {
  if(gapbuffer_capacity(line->buffer) > 0) {
    GapBufferHeader *header = gapbuffer_header(line->buffer);
    header->f_index = 0;
    header->s_index = header->capacity;
  }
}

void line_insert(Line *line, u8 codepoint) {
  assert(line);
  gapbuffer_insert(line->buffer, codepoint);
}

void line_insert_at_index(Line *line, u32 index, u8 codepoint) {
  assert(line);
  gapbuffer_move_to(line->buffer, index);
  gapbuffer_insert(line->buffer, codepoint);
}

void line_remove(Line *line) {
  assert(line);
  gapbuffer_remove(line->buffer);
}

void line_remove_at_index(Line *line, u32 index) {
  assert(line);
  gapbuffer_move_to(line->buffer, index);
  gapbuffer_remove(line->buffer);
}

void line_remove_from_front_up_to(Line *line, u32 index) {
  assert(index <= gapbuffer_size(line->buffer));
  if(line->buffer) {
    gapbuffer_move_to(line->buffer, index);
    gapbuffer_header(line->buffer)->f_index = 0;
  }
}

void line_copy(Line *des, Line *src, u32 count) {
  assert(count <= line_size(src));
  for(u32 i = 0; i < count; ++i) {
    u8 codepoint = line_get_codepoint_at(src, i);
    line_insert(des, codepoint);
  }
}

void line_copy_at(Line *des, Line *src, u32 count, u32 index) {
  assert(des);
  if(src) {
    gapbuffer_move_to(des->buffer, index);
    line_copy(des, src, count);
  }
}


u8 line_get_codepoint_at(Line *line, u32 index) {
  /* TODO: Make this iterator a macro to use in all gap buffers */
  assert(index < gapbuffer_size(line->buffer));
  if(index < gapbuffer_f_index(line->buffer)) {
    return line->buffer[index];
  } else {
    u32 offset = index - gapbuffer_f_index(line->buffer);
    return line->buffer[gapbuffer_s_index(line->buffer) + offset];
  }
}

u32 line_size(Line *line) {
  return gapbuffer_size(line->buffer);
}

void line_print(Line *line) {
  for(u32 i = 0; i < line_size(line); ++i) {
     u8 codepoint = line_get_codepoint_at(line, i);
     printf("%c", codepoint);
  }
  printf("\n");
}

File *file_create() {
  File *file = (File *)malloc(sizeof(File));
  memset(file, 0, sizeof(File));
  return file;
}

void file_destroy(File *file) {
  assert(file);
  for(u32 i = 0; i < gapbuffer_size(file->buffer); ++i) {
    Line *line = file_get_line_at(file, i);
    if(line->buffer) {
      gapbuffer_free(line->buffer);
    }
  }

  Line *free_line = file->line_first_free;
  while(free_line) {
    Line *next_to_free = free_line->next;
    free(free_line);
    free_line = next_to_free;
  }

  gapbuffer_free(file->buffer);
  free(file);
}

Line *file_line_create(File *file) {
  if(file->line_first_free) {
    Line *line = file->line_first_free;
    file->line_first_free = file->line_first_free->next;
    line_reset(line);
    return line;
  }
  return line_create();
}

void file_line_free(File *file, Line *line) {
  line->next = file->line_first_free;
  file->line_first_free = line;
}

File *file_load_from_existing_file(u8 *filename) {
  /* TODO: Maybe load the file with memcpy into the seconds gap */

  ByteArray buffer = load_entire_file(filename);
  File *file = file_create();
  if(buffer.size > 0) {
    file_insert_new_line(file);

    u32 line_count = 0;
    Line *line = file->buffer[line_count++];
    for(u32 i = 0; i < buffer.size; ++i) {
      u8 codepoint = buffer.data[i];
      if(codepoint != '\n') {
        line_insert(line, codepoint);
      } else {
        file_insert_new_line(file);
        line = file->buffer[line_count++];
      }
    }

  }
  return file;
}

void file_print(File *file) {
  printf("file lines: %d\n", gapbuffer_size(file->buffer));
  for(u32 i = 0; i < gapbuffer_f_index(file->buffer); ++i) {
    Line *line = file->buffer[i];
    for(u32 j = 0; j < gapbuffer_f_index(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    for(u32 j = gapbuffer_s_index(line->buffer); j < gapbuffer_capacity(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    printf("\n");
  }
  for(u32 i = gapbuffer_s_index(file->buffer); i < gapbuffer_capacity(file->buffer); ++i) {
    Line *line = file->buffer[i];
    for(u32 j = 0; j < gapbuffer_f_index(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    for(u32 j = gapbuffer_s_index(line->buffer); j < gapbuffer_capacity(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    printf("\n");
  }
}

void file_insert_new_line(File *file) {
  assert(file);
  Line *new_line = file_line_create(file);
  gapbuffer_insert(file->buffer, new_line);
}

void file_insert_new_line_at(File *file, u32 index) {
  assert(file);
  gapbuffer_move_to(file->buffer, index);
  file_insert_new_line(file);
}

void file_remove_line(File *file) {
  assert(file);
  Line *line = gapbuffer_get_at_gap(file->buffer);
  file_line_free(file, line);
  gapbuffer_remove(file->buffer);
}

void file_remove_line_at(File *file, u32 index) {
  assert(file);
  gapbuffer_move_to(file->buffer, index);
  file_remove_line(file);
}

Line *file_get_line_at(File *file, u32 index) {
  /* TODO: Make this iterator a macro to use in all gap buffers */
  assert(index < gapbuffer_size(file->buffer));
  if(index < gapbuffer_f_index(file->buffer)) {
    return file->buffer[index];
  } else {
    u32 offset = index - gapbuffer_f_index(file->buffer);
    return file->buffer[gapbuffer_s_index(file->buffer) + offset];
  }
}

u32 file_line_count(File *file) {
  return gapbuffer_size(file->buffer);
}

void cursor_print(Cursor cursor) {
  printf("line:%d, col:%d\n", cursor.line, cursor.col);
}

Editor *editor_create() {
  Editor *editor = (Editor *)malloc(sizeof(Editor));
  memset(editor, 0, sizeof(Editor));
  return editor;
}

void editor_destroy(Editor *editor) {
  assert(editor);
  if(editor->file) {
    file_destroy(editor->file);
  }
  free(editor);
}

void editor_step_cursor_right(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line <= file_line_count(file));
  if(cursor->col < line_size(file_get_line_at(file, cursor->line))) {
    cursor->col++;
  } else if(cursor->line < (file_line_count(file) - 1)) {
    editor_step_cursor_down(editor);
    cursor->col = 0;
  }
  cursor->save_col = cursor->col;
}

void editor_step_cursor_left(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  if(cursor->col > 0) {
    cursor->col--;
  } else if(cursor->line > 0) {
    editor_step_cursor_up(editor);
    cursor->col = line_size(file_get_line_at(file, cursor->line));
  }
  cursor->save_col = cursor->col;
}

void editor_step_cursor_up(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line <= file_line_count(file));
  if(cursor->line > 0) {
    if(cursor->line == editor->line_offset) {
      editor->line_offset--;
    }

    cursor->line--;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
  }
}

void editor_step_cursor_down(Editor *editor) {
  File *file = editor->file;
  Cursor *cursor = &editor->cursor;
  assert(cursor->line < file_line_count(file));
  if(cursor->line < (file_line_count(file) - 1)) {

    u32 total_lines_view = platform.window_height / platform.font->line_gap;
    u32 one_pass_last_view_line = MIN(editor->line_offset + total_lines_view, file_line_count(file));
    if(cursor->line == one_pass_last_view_line - 1) {
      editor->line_offset++;
    }

    cursor->line++;
    cursor->col = MIN(cursor->save_col, line_size(file_get_line_at(file, cursor->line)));
  }
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


void editor_draw_text(Painter *painter, Editor *editor) {
  File *file = editor->file;
  assert(file);

  /* TODO: Save start_x and start_y into the editor to offset calculation or do something better */
  i32 start_x = 0;
  i32 start_y = 0;

  u32 total_lines_to_render = platform.window_height / platform.font->line_gap;
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
