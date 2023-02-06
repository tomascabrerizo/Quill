#include "quill.h"

Platform platform;

Rect rect_create(i32 l, i32 r, i32 t, i32 b) {
  Rect rect = {l, r, t, b};
  return rect;
}

bool rect_contains(Rect rect, i32 x, i32 y) {
  return x >= rect.l && x < rect.r && y >= rect.t && y < rect.b;
}

bool rect_equals(Rect a, Rect b) {
  return (a.l == b.l) && (a.r == b.r) && (a.t == b.t) && (a.b == b.b);
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
