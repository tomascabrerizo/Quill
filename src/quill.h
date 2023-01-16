#ifndef _QUILL_H_
#define _QUILL_H_

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef u8 bool;
#define true 1
#define false 0

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* NOTE: All functions declare wiht QUILL_PLATFORM_API are platform dependent
 and need to be reimplemented per platform layer */
#ifndef QUILL_PLAFORM
#define QUILL_PLATFORM_API extern
#else
#define QUILL_PLATFORM_API
#endif

typedef struct Rect {
  i32 l, r, t, b;
} Rect;

Rect rect_create(i32 l, i32 r, i32 t, i32 b);
bool rect_is_valid(Rect rect);
Rect rect_intersection(Rect a, Rect b);
Rect rect_union(Rect a, Rect b);
void rect_print(Rect rect);

typedef struct ByteArray {
  u8 *data;
  u32 size;
} ByteArray;

QUILL_PLATFORM_API ByteArray load_entire_file(u8 *filename);

typedef struct BackBuffer {
  u32 *pixels;
  i32 w, h;
  Rect update_region;
  u32 bytes_per_pixel;
} BackBuffer;

BackBuffer *backbuffer_create(i32 w, i32 h, u32 bytes_per_pixel);
void backbuffer_destroy(BackBuffer *backbuffer);
void backbuffer_resize(BackBuffer *backbuffer, i32 w, i32 h);

typedef struct Glyph {
  u8 *pixels;
  u32 w, h;
  i32 advance;
  i32 bearing_x;
  i32 bearing_y;
  u16 codepoint;
} Glyph;

typedef struct Font {
  Glyph glyph_table[128];
  u32 advance;
  u32 line_gap;
  i32 ascender;
  i32 descender;
} Font;

QUILL_PLATFORM_API Font *font_load_from_file(u8 *filename, u32 font_size);
QUILL_PLATFORM_API void font_destroy(Font *font);
Glyph *font_get_glyph(u16 codepoint);

typedef struct Painter {
  u32 *pixels;
  i32 w, h;
  Rect clipping;
  Font *font;
} Painter;

Painter painter_create(BackBuffer *backbuffer);
void painter_set_font(Painter *painter, Font *font);
void painter_draw_rect(Painter *painter, Rect rect, u32 color);
void painter_draw_glyph(Painter *painter, Glyph *glyph, i32 x, i32 y, u32 color);
void painter_draw_text(Painter *painter, u8 *text, u32 size, i32 x, i32 y, u32 color);

#define GAPBUFFER_OBJECT \
  u8 *data; \
  u32 size; \
  u32 f_index; \
  u32 s_index;

#define GAPBUFFER_DEFAULT_CAPACITY 8

typedef struct GapBufferHeader {
  u32 capacity;
  u32 f_index;
  u32 s_index;
} GapBufferHeader;

void *gapbuffer_grow(void *buffer, u32 element_size);

#define gapbuffer_header(buffer) ((GapBufferHeader *)((u8 *)(buffer) - sizeof(GapBufferHeader)))

#define gapbuffer_capacity(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->capacity : 0)

#define gapbuffer_f_index(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->f_index : 0)

#define gapbuffer_s_index(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->s_index : 1)

#define gapbuffer_size(buffer) ((buffer) != 0 ? \
  (gapbuffer_f_index((buffer)) + gapbuffer_capacity((buffer)) - gapbuffer_s_index((buffer))) : 0)

#define gapbuffer_fit(buffer) ((gapbuffer_f_index((buffer)) == (gapbuffer_s_index((buffer))-1)) ? \
  (buffer) = gapbuffer_grow((buffer), sizeof(*(buffer))) : 0)

#define gapbuffer_step_foward(buffer) (gapbuffer_s_index((buffer)) < gapbuffer_capacity((buffer)) ? \
  ((buffer)[gapbuffer_f_index((buffer))] = (buffer)[gapbuffer_s_index((buffer))], \
  gapbuffer_header((buffer))->f_index++, gapbuffer_header((buffer))->s_index++) : 0)

#define gapbuffer_step_backward(buffer) (gapbuffer_f_index((buffer)) > 0 ? \
  ((buffer)[gapbuffer_s_index((buffer)) - 1] = (buffer)[gapbuffer_f_index((buffer)) - 1], \
  gapbuffer_header((buffer))->f_index--, gapbuffer_header((buffer))->s_index--) : 0)

#define gapbuffer_insert(buffer, value) (gapbuffer_fit((buffer)), \
  (buffer)[gapbuffer_f_index((buffer))] = (value), gapbuffer_header((buffer))->f_index++)

#define gapbuffer_free(buffer) (free(gapbuffer_header((buffer))))

#define gapbuffer_print_u8(buffer) \
  do {\
  for(u32 i = 0; i < gapbuffer_f_index((buffer)); ++i) { \
  printf("%c", buffer[i]); \
  } \
  printf("|"); \
  for(u32 i = gapbuffer_s_index((buffer)); i < gapbuffer_capacity((buffer)); ++i) { \
  printf("%c", buffer[i]); \
  } \
  printf("\n"); \
  } while(0)

#define gapbuffer_print_u32(buffer) \
  do {\
  for(u32 i = 0; i < gapbuffer_f_index((buffer)); ++i) { \
  printf("%d", buffer[i]); \
  } \
  printf("|"); \
  for(u32 i = gapbuffer_s_index((buffer)); i < gapbuffer_capacity((buffer)); ++i) { \
  printf("%d", buffer[i]); \
  } \
  printf("\n"); \
  } while(0)

#define gapbuffer_print_u64(buffer) \
  do {\
  for(u32 i = 0; i < gapbuffer_f_index((buffer)); ++i) { \
  printf("%ld", buffer[i]); \
  } \
  printf("|"); \
  for(u32 i = gapbuffer_s_index((buffer)); i < gapbuffer_capacity((buffer)); ++i) { \
  printf("%ld", buffer[i]); \
  } \
  printf("\n"); \
  } while(0)

/* TODO: Handle unicode characters */
typedef struct Line {
  u8 *buffer; /* NOTE: Dynamic gap buffer of characters */
} Line;

void line_free(Line *line);
void line_insert(Line *line, u8 codepoint);
void line_insert_at_index(Line *line, u32 index, u8 codepoint);
u8 line_get_codepoint_at(Line *line, u32 index);
u32 line_size(Line *line);

typedef struct File {
  Line *buffer;
} File;

File *file_create();
File *file_load_from_existing_file(u8 *filename);
void file_destroy(File *file);
void file_insert_new_line(File *file);
void file_print(File *file);
Line *file_get_line_at(File *file, u32 index);
u32 file_line_count(File *file);

typedef struct Cursor {
  u32 col;
  u32 save_col;
  u32 line;
} Cursor;


typedef struct Editor {
  File *file;
  Cursor cursor;
  u32 col_offset;
  u32 line_offset;
} Editor;

Editor *editor_create();
void editor_destroy(Editor *editor);
void editor_step_cursor_right(Editor *editor);
void editor_step_cursor_left(Editor *editor);
void editor_step_cursor_up(Editor *editor);
void editor_step_cursor_down(Editor *editor);
void editor_cursor_insert(Editor *editor, u8 codepoint);
void editor_draw_text(Painter *painter, Editor *editor);

#endif /* _QUILL_H_ */
