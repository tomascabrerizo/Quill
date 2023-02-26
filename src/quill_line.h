#ifndef _QUILL_LINE_H_
#define _QUILL_LINE_H_

#include "quill.h"
#include "quill_element.h"

/* TODO: Handle unicode characters */
typedef struct Line {
  u8 *buffer;
  struct Line *next;
} Line;

Line *line_create(void);
void line_destroy(Line *line);
void line_reset(Line *line);
void line_insert(Line *line, u8 codepoint);
void line_insert_at_index(Line *line, u32 index, u8 codepoint);
void line_remove(Line *line);
void line_remove_at_index(Line *line, u32 index);
void line_remove_from_front_up_to(Line *line, u32 index);
void line_copy(Line *des, Line *src, u32 count);
void line_copy_at(Line *des, Line *src, u32 count, u32 index);
u8 line_get_codepoint_at(Line *line, u32 index);
u32 line_size(Line *line);

void line_print(Line *line);

#endif /* _QUILL_LINE_H_ */
