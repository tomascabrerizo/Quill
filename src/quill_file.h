#ifndef _QUILL_FILE_H_
#define _QUILL_FILE_H_

#include "quill.h"
#include "quill_element.h"

typedef struct File {
  QUILL_ELEMENT

  u8 *name;
  struct Line **buffer;
  struct Line *line_first_free;
} File;

File *file_create(Element *element);

struct Line *file_line_create(File *file);
void file_line_free(File *file, struct Line *line);

File *file_load_from_existing_file(Element *parent, u8 *filename);
void file_insert_new_line(File *file);
void file_insert_new_line_at(File *file, u32 index);
void file_remove_line(File *file);
void file_remove_line_at(File *file, u32 index);
void file_print(File *file);
struct Line *file_get_line_at(File *file, u32 index);
u32 file_line_count(File *file);

#endif /* _QUILL_FILE_H_ */
