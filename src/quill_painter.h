#ifndef _QUILL_PAINTER_H_
#define _QUILL_PAINTER_H_

#include "quill.h"

struct Line;
struct Tokenizer;

typedef struct Painter {
  u32 *pixels;
  i32 w, h;
  Rect clipping;
  Font *font;
} Painter;

Painter painter_create(BackBuffer *backbuffer);
void painter_set_font(Painter *painter, Font *font);
void painter_draw_rect(Painter *painter, Rect rect, u32 color);
void painter_draw_rect_outline(Painter *painter, Rect rect, u32 color);
void painter_draw_glyph(Painter *painter, Glyph *glyph, i32 x, i32 y, u32 color);
void painter_draw_text(Painter *painter, u8 *text, u32 size, i32 x, i32 y, u32 color);
void painter_draw_line(Painter *painter, struct Line *line, i32 x, i32 y, u32 color);

#endif /* _QUILL_PAINTER_H_ */
