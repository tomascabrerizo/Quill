#include "quill_line.h"
extern Platform platform;

Line *line_create(void) {
  Line *line = (Line *)malloc(sizeof(Line));
  memset(line, 0, sizeof(Line));

  return line;
}

void line_destroy(Line *line) {
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
