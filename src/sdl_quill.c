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

void freetype_shutdow() {
  FT_Done_FreeType(freetype_library);
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
  font->ascender = face->size->metrics.ascender >> 6;
  font->descender = face->size->metrics.descender >> 6;
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

  FT_Done_Face(face);

  return font;
}

QUILL_PLATFORM_API void font_destroy(Font *font) {
  /* TODO: Free all bitmap of each used glyph */
  free(font);
}

/* NOTE: Platfom is a gobal variable define in quill.c that the platform need to implements */
extern Platform platform;

/* NOTE: Each platfrom need its main function */

int main(void) {
  /* NOTE: Window and Bacbuffer initialization */
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("Quill", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE);
  SDL_ShowWindow(window);

  /* TODO: Create and load Quill icon */
  SDL_Surface *icon = SDL_LoadBMP("./data/quill_icon.bmp");
  SDL_SetWindowIcon(window, icon);

  SDL_Surface *window_surface = SDL_GetWindowSurface(window);
  assert(window_surface->format->BytesPerPixel == 4);
  assert(window_surface->format->format == SDL_PIXELFORMAT_RGB888);

  platform.backbuffer = backbuffer_create(window_surface->w, window_surface->h, window_surface->format->BytesPerPixel);

  Editor *editor0 = editor_create();
  editor0->file = file_load_from_existing_file((u8 *)"./src/quill.c");

  Editor *editor1 = editor_create();
  editor1->file = file_load_from_existing_file((u8 *)"./src/quill.h");

  Editor *editor = editor1;
  SDL_SetWindowTitle(window, (const char *)editor->file->name);

  Font *font = font_load_from_file((u8 *)"/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 14);
  platform.font = font;

  /* NOTE: Platform events */
  SDL_Event e;
  while(SDL_WaitEvent(&e)) {
    if(e.type == SDL_WINDOWEVENT) {
      BackBuffer *backbuffer = platform.backbuffer;
      if(e.window.event == SDL_WINDOWEVENT_SHOWN) {

        editor0->rect = rect_create(0, backbuffer->w/2, 0, backbuffer->h);
        editor_redraw_lines(editor0, 0, (editor0->rect.b - editor0->rect.t) / platform.font->line_gap);
        editor1->rect = rect_create(backbuffer->w/2, backbuffer->w, 0, backbuffer->h);
        editor_redraw_lines(editor1, 0, (editor1->rect.b - editor1->rect.t) / platform.font->line_gap);

      } else if(e.window.event == SDL_WINDOWEVENT_RESIZED) {
        backbuffer_resize(backbuffer, e.window.data1, e.window.data2);

        editor0->rect = rect_create(0, backbuffer->w/2, 0, backbuffer->h);
        editor_redraw_lines(editor0, 0, (editor0->rect.b - editor0->rect.t) / platform.font->line_gap);
        editor1->rect = rect_create(backbuffer->w/2, backbuffer->w, 0, backbuffer->h);
        editor_redraw_lines(editor1, 0, (editor1->rect.b - editor1->rect.t) / platform.font->line_gap);

      } else if(e.window.event == SDL_WINDOWEVENT_EXPOSED) {

        Painter painter = painter_create(backbuffer);

        painter_set_font(&painter, font);
        if(editor0->redraw) {
          editor_draw_text(&painter, editor0);
        }
        if(editor1->redraw) {
          editor_draw_text(&painter, editor1);
        }

        Rect old_clipping = painter.clipping;
        painter.clipping = backbuffer->debug_update_region;
        painter_draw_rect_outline(&painter, backbuffer->debug_update_region, 0xff00ff);
        painter_draw_rect_outline(&painter, backbuffer->last_update_region, 0x202020);
        painter_draw_rect_outline(&painter, backbuffer->update_region, 0x00ff00);
        painter.clipping = old_clipping;

        //printf("---------------------------------\n");
        //rect_print(backbuffer->debug_update_region);
        //rect_print(backbuffer->last_update_region);
        //rect_print(backbuffer->update_region);

        window_surface = SDL_GetWindowSurface(window);
        SDL_Surface *backbuffer_surface = SDL_CreateRGBSurfaceWithFormatFrom(backbuffer->pixels,
                                                                             backbuffer->w,
                                                                             backbuffer->h,
                                                                             backbuffer->bytes_per_pixel*8,
                                                                             backbuffer->w * backbuffer->bytes_per_pixel,
                                                                             window_surface->format->format);

        SDL_Rect update_rect = {backbuffer->debug_update_region.l,  backbuffer->debug_update_region.t,
                                backbuffer->debug_update_region.r - backbuffer->debug_update_region.l,
                                backbuffer->debug_update_region.b - backbuffer->debug_update_region.t};

        SDL_BlitSurface(backbuffer_surface, &update_rect, window_surface, &update_rect);
        SDL_FreeSurface(backbuffer_surface);
        SDL_UpdateWindowSurface(window);
        //SDL_UpdateWindowSurfaceRects(window, (const SDL_Rect *)&update_rect, 1);
      }
    } else if(e.type == SDL_TEXTINPUT) {
      u8 codepoint = (u8)e.text.text[0];
      editor_cursor_insert(editor, codepoint);

      assert(editor);
      editor->redraw = true;
      backbuffer_set_update_region(platform.backbuffer, editor->rect);
      SDL_Event event;
      event.type = SDL_WINDOWEVENT;
      event.window.event = SDL_WINDOWEVENT_EXPOSED;
      assert(SDL_PushEvent(&event) == 1);

    } else if(e.type == SDL_KEYDOWN) {

      if((e.key.keysym.scancode == SDL_SCANCODE_RIGHT) ||
         (e.key.keysym.scancode == SDL_SCANCODE_LEFT) ||
         (e.key.keysym.scancode == SDL_SCANCODE_UP) ||
         (e.key.keysym.scancode == SDL_SCANCODE_DOWN)) {
        editor_update_selected(editor, e.key.keysym.mod & KMOD_SHIFT);
      }

      if(e.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
        editor_step_cursor_right(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_LEFT) {
        editor_step_cursor_left(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_UP) {
        editor_step_cursor_up(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_DOWN) {
        editor_step_cursor_down(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
        editor_cursor_remove(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_DELETE) {
        editor_cursor_remove_right(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
        editor_cursor_insert_new_line(editor);
      }

      assert(editor);
      editor->redraw = true;
      backbuffer_set_update_region(platform.backbuffer, editor->rect);
      SDL_Event event;
      event.type = SDL_WINDOWEVENT;
      event.window.event = SDL_WINDOWEVENT_EXPOSED;
      assert(SDL_PushEvent(&event) == 1);

    } else if(e.type == SDL_MOUSEBUTTONDOWN) {
      if(e.button.button == SDL_BUTTON_LEFT) {
        if(rect_contains(editor0->rect, e.button.x, e.button.y)) {
          SDL_SetWindowTitle(window, (const char *)editor0->file->name);
          editor = editor0;

        } else if(rect_contains(editor1->rect, e.button.x, e.button.y)) {
          SDL_SetWindowTitle(window, (const char *)editor1->file->name);
          editor = editor1;
        }

        assert(editor);
        editor->redraw = true;
        backbuffer_set_update_region(platform.backbuffer, editor->rect);
        SDL_Event event;
        event.type = SDL_WINDOWEVENT;
        event.window.event = SDL_WINDOWEVENT_EXPOSED;
        assert(SDL_PushEvent(&event) == 1);
      }
    } else if(e.type == SDL_QUIT) {
      printf("Quitting application\n");
      break;
    }
  }

  editor_destroy(editor0);
  editor_destroy(editor1);
  backbuffer_destroy(platform.backbuffer);
  font_destroy(font);

  return 0;
}
