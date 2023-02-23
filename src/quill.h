#ifndef _QUILL_H_
#define _QUILL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* TODO: Create quill_types.h and quill_data_structures.h */
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

#define array_count(array) (sizeof((array)) / sizeof((array)[0]))

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
bool rect_contains(Rect rect, i32 x, i32 y);
bool rect_equals(Rect a, Rect b);
bool rect_is_valid(Rect rect);
Rect rect_intersection(Rect a, Rect b);
Rect rect_union(Rect a, Rect b);
void rect_print(Rect rect);



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

typedef struct ByteArray {
  u8 *data;
  u32 size;
} ByteArray;

QUILL_PLATFORM_API ByteArray load_entire_file(u8 *filename);
QUILL_PLATFORM_API struct Folder *platform_load_folder(u8 *foldername);

typedef struct Platform {
  BackBuffer *backbuffer;
  u32 mouse_pos_x;
  u32 mouse_pos_y;
  Font *font;
  u8 *temp_clipboard;
  void *data;
} Platform;

QUILL_PLATFORM_API u8 *platform_get_clipboard();
QUILL_PLATFORM_API void platform_free_clipboard(u8 *buffer);
QUILL_PLATFORM_API void platform_set_clipboard(u8 *buffer);

QUILL_PLATFORM_API void platform_end_draw(BackBuffer *backbuffer);
QUILL_PLATFORM_API void platform_temp_clipboard_push(Platform *platform, u8 value);
QUILL_PLATFORM_API void platform_temp_clipboard_clear(Platform *platform);



#endif /* _QUILL_H_ */
