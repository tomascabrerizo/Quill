#include "quill.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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


void backbuffer_alloc(BackBuffer *backbuffer, i32 w, i32 h, u32 bytes_per_pixel) {
  backbuffer->w = w;
  backbuffer->h = h;
  backbuffer->bytes_per_pixel = bytes_per_pixel;
  backbuffer->update_region = rect_create(0, w, 0, h);
  backbuffer->pixels = (u32 *)malloc(w * h * bytes_per_pixel);
}

void backbuffer_free(BackBuffer *backbuffer) {
  free(backbuffer->pixels);
}

void backbuffer_realloc(BackBuffer *backbuffer, i32 w, i32 h) {
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
    printf("buffer grow to:%d elements, %d bytes\n", new_buffer_size, new_buffer_size * element_size);
    return (header + 1);
  }
}

void textbuffer_alloc(TextBuffer *textbuffer, u32 size) {
  assert(textbuffer);
  textbuffer->data = (u8 *)malloc(size);
  textbuffer->size = size;
  textbuffer->f_index = 0;
  textbuffer->s_index = size;
}

void textbuffer_free(TextBuffer *textbuffer) {
  assert(textbuffer);
  free(textbuffer->data);
}

void textbuffer_step_gap_foward(TextBuffer *textbuffer) {
  assert(textbuffer);
  if(textbuffer->s_index < textbuffer->size) {
    textbuffer->data[textbuffer->f_index] = textbuffer->data[textbuffer->s_index];
    textbuffer->f_index++;
    textbuffer->s_index++;
  }
}

void textbuffer_step_gap_backward(TextBuffer *textbuffer) {
  assert(textbuffer);
  if(textbuffer->f_index > 0) {
    textbuffer->data[textbuffer->s_index - 1] = textbuffer->data[textbuffer->f_index - 1];
    textbuffer->f_index--;
    textbuffer->s_index--;
  }
}

void textbuffer_grow(TextBuffer *textbuffer) {
  assert(textbuffer);
  u32 new_buffer_size = textbuffer->size * 2;
  textbuffer->data = (u8 *)realloc(textbuffer->data, new_buffer_size);
  u32 second_gap_size = (textbuffer->size - textbuffer->s_index);
  u8 *des = textbuffer->data + new_buffer_size - second_gap_size;
  u8 *src = textbuffer->data + textbuffer->size - second_gap_size;
  memmove(des, src, second_gap_size);
  textbuffer->s_index = new_buffer_size - second_gap_size;
  textbuffer->size = new_buffer_size;
  printf("buffer grow to:%d\n", new_buffer_size);
}

void textbuffer_insert(TextBuffer *textbuffer, u16 character) {
  assert(textbuffer);
  assert(textbuffer->f_index < textbuffer->s_index);
  assert(textbuffer->f_index < textbuffer->size);
  if(textbuffer->f_index == (textbuffer->s_index - 1)) {
    textbuffer_grow(textbuffer);
  }
  assert(textbuffer->f_index < (textbuffer->s_index - 1));
  textbuffer->data[textbuffer->f_index] = (u8)character;
  textbuffer->f_index++;
}

void textbuffer_print(TextBuffer *textbuffer) {
  for(u32 i = 0; i < textbuffer->f_index; ++i) {
    printf("%c", (char)textbuffer->data[i]);
  }
  printf("|");
  for(u32 i = textbuffer->s_index; i < textbuffer->size; ++i) {
    printf("%c", (char)textbuffer->data[i]);
  }
  printf("\n");
}

static inline Line line_empty_line(void) {
  Line line;
  memset(&line, 0, sizeof(line));
  return line;
}

void line_free(Line *line) {
  assert(line);
  gapbuffer_free(line->buffer);
}

void line_insert(Line *line, u8 codepoint) {
  assert(line);
  gapbuffer_insert(line->buffer, codepoint);
}

void line_insert_at_index(Line *line, u32 index, u8 codepoint) {
  assert(line);
  assert(index < gapbuffer_capacity(line->buffer));
  i32 distance = (i32)index - (i32)gapbuffer_f_index(line->buffer);
  if(distance > 0) {
    for(u32 i = gapbuffer_f_index(line->buffer); i < index; ++i) {
      gapbuffer_step_foward(line->buffer);
    }
  } else if(distance < 0) {
    for(u32 i = gapbuffer_f_index(line->buffer); i > index; --i) {
      gapbuffer_step_backward(line->buffer);
    }
  }
  line_insert(line, codepoint);
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



File *file_create() {
  File *file = (File *)malloc(sizeof(File));
  memset(file, 0, sizeof(File));
  return file;
}

void file_destroy(File *file) {
  assert(file);
  for(u32 i = 0; i < gapbuffer_size(file->buffer); ++i) {
    Line *line = &file->buffer[i];
    if(line->buffer) {
      gapbuffer_free(line->buffer);
    }
  }
  gapbuffer_free(file->buffer);
  free(file);
}

File *file_load_from_existing_file(u8 *filename) {
  ByteArray buffer = load_entire_file(filename);
  File *file = file_create();
  if(buffer.size > 0) {
    file_insert_new_line(file);

  }
  /* TODO: Maybe load the file with memcpy into the seconds gap */
  u32 line_count = 0;
  Line *line = &file->buffer[line_count++];
  for(u32 i = 0; i < buffer.size; ++i) {
    u8 codepoint = buffer.data[i];
    if(codepoint != '\n') {
      line_insert(line, codepoint);
    } else {
      file_insert_new_line(file);
      line = &file->buffer[line_count++];
    }
  }
  return file;
}

void file_print(File *file) {
  printf("file lines: %d\n", gapbuffer_size(file->buffer));
  for(u32 i = 0; i < gapbuffer_f_index(file->buffer); ++i) {
    Line *line = &file->buffer[i];
    for(u32 j = 0; j < gapbuffer_f_index(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    for(u32 j = gapbuffer_s_index(line->buffer); j < gapbuffer_capacity(line->buffer); ++j) {
      printf("%c", line->buffer[j]);
    }
    printf("\n");
  }
  for(u32 i = gapbuffer_s_index(file->buffer); i < gapbuffer_capacity(file->buffer); ++i) {
    Line *line = &file->buffer[i];
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

  gapbuffer_insert(file->buffer, line_empty_line());
}

Line *file_get_line_at(File *file, u32 index) {
  /* TODO: Make this iterator a macro to use in all gap buffers */
  assert(index < gapbuffer_size(file->buffer));
  if(index < gapbuffer_f_index(file->buffer)) {
    return &file->buffer[index];
  } else {
    u32 offset = index - gapbuffer_f_index(file->buffer);
    return &file->buffer[gapbuffer_s_index(file->buffer) + offset];
  }
}

u32 file_line_count(File *file) {
  return gapbuffer_size(file->buffer);
}


Editor *editor_alloc(void) {
  Editor *editor = (Editor *)malloc(sizeof(Editor));
  editor->font = font_load_from_file((u8 *)"/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 12);
  return editor;
}

void editor_free(Editor *editor) {
  assert(editor);
  backbuffer_free(&editor->backbuffer);
  free(editor);
}

void editor_alloc_backbuffer(Editor *editor, i32 w, i32 h, u32 bytes_per_pixel) {
  backbuffer_alloc(&editor->backbuffer, w, h, bytes_per_pixel);
}

void editor_realloc_backbuffer(Editor *editor, i32 w, i32 h) {
  backbuffer_realloc(&editor->backbuffer, w, h);
}

void editor_render_textbuffer(Editor *editor, u32 offset_y) {
  Painter painter = painter_create(&editor->backbuffer);
  painter_set_font(&painter, editor->font);

  u8 *text1 = (u8 *)"Quill Text Editor!";
  painter_draw_text(&painter, text1, strlen((char *)text1), 20, editor->font->line_gap*1, 0xc0c0c0);

  (void)editor;
  (void)offset_y;
}


