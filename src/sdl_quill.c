#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <assert.h>

#define QUILL_PLATFORM
#include "quill.h"

/* NOTE: Platform helper functions */
static bool freetype_is_initialize;
static FT_Library freetype_library;

void freetype_initialize() {
  FT_Error error = FT_Init_FreeType(&freetype_library);
  if(error) {
    printf("Cannot initialize freetype\n");
    exit(-1);
  } else {
    printf("Freetype is initialize\n");
  }
}

/* NOTE: Function from the platform to the program */

QUILL_PLATFORM_API ByteArray load_entire_file(u8 *filename) {
  FILE *file = fopen((char *)filename, "rb");
  if(!file) {
    printf("Cannot load file: %s\n", filename);
    exit(-1);
  }
  fseek(file, 0, SEEK_END);
  u64 file_size = (u64)ftell(file);
  fseek(file, 0, SEEK_SET);
  u8 *buffer = (u8 *)malloc(file_size);
  fread(buffer, file_size, 1, file);
  ByteArray result;
  result.data = buffer;
  result.size = file_size;
  return result;
}

QUILL_PLATFORM_API Font *font_load_from_file(u8 *filename, u32 font_size) {
  if(!freetype_is_initialize) {
    freetype_initialize();
    freetype_is_initialize = true;
  }
  assert(freetype_is_initialize);

  FT_Face face;
  FT_Error error = FT_New_Face(freetype_library, (const char *)filename, 0, &face);
  if(error) {
    printf("Cannot load face: %s\n", filename);
    exit(-1);
  }

  assert(face->face_flags & FT_FACE_FLAG_SCALABLE);
  assert(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH);

  u32 resolution = 96;
  error = FT_Set_Char_Size(face, 0, font_size << 6, 0, (u32)resolution);
  if(error) {
    printf("Cannot set font size\n");
    exit(-1);
  }

  Font *font = (Font *)malloc(sizeof(Font));
  font->line_gap = face->size->metrics.height >> 6;
  font->advance = face->size->metrics.max_advance >> 6;
  for(u16 codepoint = (u16)' '; codepoint <= (u16)'~'; ++codepoint)
  {
    Glyph *glyph = &font->glyph_table[codepoint];

    FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)codepoint);
    FT_Error freetype_error = FT_Load_Glyph(face, glyph_index, 0);
    if(freetype_error) {
      printf("Cannot load glyph: [%d]\n", glyph_index);
      exit(-1);
    }
    /* NOTE: Render glyph to bitmap */
    freetype_error = FT_Render_Glyph(face->glyph, 0);
    if(freetype_error) {
      printf("Cannot render glyph: [%d]\n", glyph_index);
      exit(-1);

    }
    assert(face->glyph->format == FT_GLYPH_FORMAT_BITMAP);

    assert((i32)face->glyph->bitmap.width == face->glyph->bitmap.pitch);
    u32 glyph_bitmap_size = face->glyph->bitmap.rows*face->glyph->bitmap.pitch;
    glyph->pixels = (u8 *)malloc(glyph_bitmap_size);
    memcpy(glyph->pixels, face->glyph->bitmap.buffer, glyph_bitmap_size);
    glyph->w = face->glyph->bitmap.width;
    glyph->h = face->glyph->bitmap.rows;

    glyph->bearing_x = face->glyph->bitmap_left;
    glyph->bearing_y = face->glyph->bitmap_top;
    glyph->advance = (u32)(face->glyph->advance.x >> 6);
    assert((u32)glyph->advance == font->advance);
    glyph->codepoint = codepoint;
  }

  return font;
}

QUILL_PLATFORM_API void font_free(Font *font) {
  /* TODO: Free all bitmap of each used glyph */
  free(font);
}

/* NOTE: Each platfrom need its main function */

int main(void) {
  /* NOTE: Window and Bacbuffer initialization */
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("Quill", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE);
  SDL_ShowWindow(window);

  SDL_Surface *window_surface = SDL_GetWindowSurface(window);
  assert(window_surface->format->BytesPerPixel == 4);
  assert(window_surface->format->format == SDL_PIXELFORMAT_RGB888);

  Editor *editor = editor_alloc();
  editor_alloc_backbuffer(editor, window_surface->w, window_surface->h, window_surface->format->BytesPerPixel);

  /* NOTE: New Generic gapbuffer test */
  u64 *buffer = 0;

  gapbuffer_insert(buffer, 1);
  gapbuffer_insert(buffer, 2);
  gapbuffer_insert(buffer, 3);
  gapbuffer_insert(buffer, 4);
  gapbuffer_print_u64(buffer);

  gapbuffer_step_backward(buffer);
  gapbuffer_step_backward(buffer);
  gapbuffer_print_u64(buffer);

  gapbuffer_insert(buffer, 5);
  gapbuffer_insert(buffer, 7);
  gapbuffer_insert(buffer, 8);
  gapbuffer_insert(buffer, 9);
  gapbuffer_print_u64(buffer);

  for(i32 i = 0; i < 100; ++i)
    gapbuffer_step_backward(buffer);
  gapbuffer_print_u64(buffer);

  gapbuffer_insert(buffer, 0);
  gapbuffer_insert(buffer, 0);
  gapbuffer_insert(buffer, 0);
  gapbuffer_print_u64(buffer);

  for(i32 i = 0; i < 100; ++i)
    gapbuffer_step_foward(buffer);
  gapbuffer_print_u64(buffer);

  gapbuffer_insert(buffer, 0);
  gapbuffer_insert(buffer, 0);
  gapbuffer_insert(buffer, 0);
  gapbuffer_print_u64(buffer);

  printf("buffer size: %d\n", gapbuffer_size(buffer));

  /* NOTE: Loadin file into GapBuffer test */
  File *file = file_load_from_existing_file((u8 *)"./test.txt");
  file_print(file);

  /* NOTE: Platform events */
  SDL_Event e;
  while(SDL_WaitEvent(&e)) {
    if(e.type == SDL_WINDOWEVENT) {
      if(e.window.event == SDL_WINDOWEVENT_RESIZED) {
        u32 w = e.window.data1;
        u32 h = e.window.data2;
        editor_realloc_backbuffer(editor, w, h);
      } else if(e.window.event == SDL_WINDOWEVENT_EXPOSED) {
        BackBuffer backbuffer = editor->backbuffer;

        if(rect_is_valid(backbuffer.update_region)) {
          /* NOTE: Painter test */
          Painter painter = painter_create(&backbuffer);
          painter_draw_rect(&painter, backbuffer.update_region, 0x202020);

          i32 pen_x = 20;
          i32 pen_y = 20 + editor->font->line_gap;
          for(u32 i = 0; i < file_line_count(file); ++i) {
            Line *line = file_get_line_at(file, i);
            for(u32 j = 0; j < line_size(line); ++j) {
              u8 codepoint = line_get_codepoint_at(line, j);
              painter_draw_glyph(&painter, &editor->font->glyph_table[codepoint], pen_x, pen_y, 0xc0c0c0);
              pen_x += editor->font->advance;
            }
            pen_x = 20;
            pen_y += editor->font->line_gap;
          }

          window_surface = SDL_GetWindowSurface(window);
          SDL_Surface *backbuffer_surface = SDL_CreateRGBSurfaceWithFormatFrom(backbuffer.pixels,
                                                                               backbuffer.w,
                                                                               backbuffer.h,
                                                                               backbuffer.bytes_per_pixel*8,
                                                                               backbuffer.w * backbuffer.bytes_per_pixel,
                                                                               window_surface->format->format);

          SDL_Rect update_rect = {backbuffer.update_region.l, backbuffer.update_region.t,
                                  backbuffer.update_region.r - backbuffer.update_region.l,
                                  backbuffer.update_region.b - backbuffer.update_region.t};

          SDL_BlitSurface(backbuffer_surface, &update_rect, window_surface, &update_rect);
          SDL_FreeSurface(backbuffer_surface);
          SDL_UpdateWindowSurface(window);
          //SDL_UpdateWindowSurfaceRects(window, (const SDL_Rect *)&update_rect, 1);
        }
      }
    } else if(e.type == SDL_TEXTINPUT) {
    } else if(e.type == SDL_QUIT) {
      printf("Quitting application\n");
      break;
    }
  }

  file_destroy(file);
  editor_free(editor);


  return 0;
}
