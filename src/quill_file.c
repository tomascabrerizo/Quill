#include "quill_file.h"
#include "quill_line.h"

extern Platform platform;

static int file_default_message_handler(struct Element *element, Message message, void *data) {
  /* TODO: Implements default line message handler */
  (void)element; (void)message; (void)data;
  return 0;
}

static void file_destroy(Element *element) {
  File *file = (File *)element;
  gapbuffer_free(file->buffer);
}

File *file_create(Element *parent) {
  File *file = (File *)element_create(sizeof(File), parent, file_default_message_handler);
  element_set_user_element_destroy((Element *)file, file_destroy);
  element_set_type((Element *)file, ELEMENT_FILE);
  return file;
}

Line *file_line_create(File *file) {
  if(file->line_first_free) {
    Line *line = file->line_first_free;
    file->line_first_free = file->line_first_free->next;
    line_reset(line);
    return line;
  }
  return line_create((Element *)file);
}

void file_line_free(File *file, Line *line) {
  line->next = file->line_first_free;
  file->line_first_free = line;
}

File *file_load_from_existing_file(Element *parent, u8 *filename) {
  /* TODO: Maybe load the file with memcpy into the seconds gap */

  ByteArray buffer = load_entire_file(filename);
  File *file = file_create(parent);
  file->name = filename;
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