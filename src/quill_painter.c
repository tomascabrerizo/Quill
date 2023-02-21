#include "quill_painter.h"
#include "quill_line.h"
#include "quill_tokenizer.h"

extern Platform platform;

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
  if(!rect_is_valid(rect)) {
    return;
  }
  for(i32 yy = rect.t; yy < rect.b; ++yy) {
    for(i32 xx = rect.l; xx < rect.r; ++xx) {
      painter->pixels[yy*painter->w+xx] = color;
    }
  }
}

void painter_draw_rect_outline(Painter *painter, Rect rect, u32 color) {
  /* NOTE: This function draw a rect outline but is not clipping the outline
    it will create a outline of the clipping intersection */
  rect = rect_intersection(rect, painter->clipping);
  if(!rect_is_valid(rect)) {
    return;
  }
  u32 outline = 1;
  Rect t = rect_create(rect.l, rect.r, rect.t, rect.t + outline);
  Rect b = rect_create(rect.l, rect.r, rect.b - outline, rect.b);
  Rect l = rect_create(rect.l, rect.l + outline, rect.t, rect.b);
  Rect r = rect_create(rect.r - outline, rect.r, rect.t, rect.b);
  painter_draw_rect(painter, t, color);
  painter_draw_rect(painter, b, color);
  painter_draw_rect(painter, l, color);
  painter_draw_rect(painter, r, color);

}

void painter_draw_glyph(Painter *painter, Glyph *glyph, i32 x, i32 y, u32 color) {
  x += glyph->bearing_x;
  y -= glyph->bearing_y;

  Rect rect = rect_create(x, x + glyph->w, y, y + glyph->h);
  rect = rect_intersection(rect, painter->clipping);
  if(!rect_is_valid(rect)) {
    return;
  }

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

void painter_draw_token(Painter *painter, Token *token, i32 x, i32 y, u32 color) {
  /* TODO: Create defines to set a color theme */

  switch(token->type) {
  case TOKEN_TYPE_WORD: { color = color; } break;
  case TOKEN_TYPE_KEYWORD: { color = 0xbb8800; } break;
  case TOKEN_TYPE_STRING: { color = 0x888800; } break;
  case TOKEN_TYPE_NUBER: { color = 0x00ff00; } break;
  case TOKEN_TYPE_COMMENT: { color = 0x666666; } break;
  default: {} break;
  }

  for(u32 i = token->start; i < token->end; ++i) {
    u8 codepoint = line_get_codepoint_at(token->line, i);
    Glyph *glyph = &painter->font->glyph_table[codepoint];
    painter_draw_glyph(painter, glyph, x, y, color);
    x += platform.font->advance;
  }

}

#if 1
void painter_draw_line(Painter *painter, struct Line *line, i32 x, i32 y, u32 color) {
  Tokenizer tokenizer = tokenizer_init(line);
  Token token;
  while(tokenizer_next_token(&tokenizer, &token)) {
    painter_draw_token(painter, &token, x, y, color);
    x += (token.end - token.start) * platform.font->advance;
  }
}
#elif
void painter_draw_line(Painter *painter, struct Line *line, i32 x, i32 y, u32 color) {
  for(u32 i = 0; i < line_size(line); ++i) {
    u8 codepoint = line_get_codepoint_at(line, i);
    Glyph *glyph = &painter->font->glyph_table[codepoint];
    painter_draw_glyph(painter, glyph, x, y, color);
    x += platform.font->advance;
  }
}
#endif
