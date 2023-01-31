#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <assert.h>

#include "quill_file.h"
#include "quill_editor.h"
#include "quill_application.h"

#define QUILL_PLATFORM

/* NOTE: Platfom is a gobal variable define in quill.c that the platform need to implements */
extern Platform platform;

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

QUILL_PLATFORM_API void platform_end_draw(BackBuffer *backbuffer) {
  assert(platform.data);
  SDL_Window *window = (SDL_Window *)platform.data;
  SDL_Surface *window_surface = SDL_GetWindowSurface(window);
  SDL_Surface *backbuffer_surface = SDL_CreateRGBSurfaceWithFormatFrom(backbuffer->pixels,
                                                                       backbuffer->w,
                                                                       backbuffer->h,
                                                                       backbuffer->bytes_per_pixel*8,
                                                                       backbuffer->w * backbuffer->bytes_per_pixel,
                                                                       window_surface->format->format);

  SDL_Rect update_rect = {backbuffer->update_region.l,  backbuffer->update_region.t,
                          backbuffer->update_region.r - backbuffer->update_region.l,
                          backbuffer->update_region.b - backbuffer->update_region.t};

  SDL_BlitSurface(backbuffer_surface, &update_rect, window_surface, &update_rect);
  SDL_FreeSurface(backbuffer_surface);
  SDL_UpdateWindowSurface(window);
}

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

  platform.data = (void *)window;
  platform.backbuffer = backbuffer_create(window_surface->w, window_surface->h, window_surface->format->BytesPerPixel);
  platform.font = font_load_from_file((u8 *)"/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 14);;

  Application *application = application_create(platform.backbuffer);
  Editor *editor0 = editor_create(&application->element);
  editor0->file = file_load_from_existing_file((u8 *)"./src/quill.c");
  Editor *editor1 = editor_create(&application->element);
  editor1->file = file_load_from_existing_file((u8 *)"./src/quill.h");
  application_set_current_editor(application, editor0);
  Editor *editor = editor0;

  SDL_SetWindowTitle(window, (const char *)editor->file->name);

  /* NOTE: Platform events */
  SDL_Event e;
  while(SDL_WaitEvent(&e)) {
    if(e.type == SDL_WINDOWEVENT) {
      BackBuffer *backbuffer = platform.backbuffer;
      if(e.window.event == SDL_WINDOWEVENT_SHOWN) {
        Rect backbuffer_rect = rect_create(0, backbuffer->w, 0, backbuffer->h);
        element_resize(application, backbuffer_rect);
        element_update(application);

      } else if(e.window.event == SDL_WINDOWEVENT_RESIZED) {
        backbuffer_resize(backbuffer, e.window.data1, e.window.data2);
        Rect backbuffer_rect = rect_create(0, backbuffer->w, 0, backbuffer->h);
        element_resize(application, backbuffer_rect);
        element_update(application);

      } else if(e.window.event == SDL_WINDOWEVENT_EXPOSED) {
        platform_end_draw(platform.backbuffer);
      }
    } else if(e.type == SDL_TEXTINPUT) {
      u8 codepoint = (u8)e.text.text[0];
      editor_cursor_insert(editor, codepoint);
      element_message(application, MESSAGE_TEXTINPUT, 0);

    } else if(e.type == SDL_KEYDOWN) {

      if((e.key.keysym.scancode == SDL_SCANCODE_RIGHT) ||
         (e.key.keysym.scancode == SDL_SCANCODE_LEFT) ||
         (e.key.keysym.scancode == SDL_SCANCODE_UP) ||
         (e.key.keysym.scancode == SDL_SCANCODE_DOWN)) {
        editor_update_selected(editor, e.key.keysym.mod & KMOD_SHIFT);
      }

      if(e.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
        element_message(application, MESSAGE_KEYDOWN, EDITOR_KEY_RIGHT);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_LEFT) {
        element_message(application, MESSAGE_KEYDOWN, (void *)EDITOR_KEY_LEFT);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_UP) {
        element_message(application, MESSAGE_KEYDOWN, (void *)EDITOR_KEY_UP);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_DOWN) {
        element_message(application, MESSAGE_KEYDOWN, (void *)EDITOR_KEY_DOWN);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
        editor_cursor_remove(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_DELETE) {
        editor_cursor_remove_right(editor);
      } else if(e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
        editor_cursor_insert_new_line(editor);
      }

    } else if(e.type == SDL_MOUSEBUTTONDOWN) {
      if(e.button.button == SDL_BUTTON_LEFT) {
        if(rect_contains(editor0->element.rect, e.button.x, e.button.y)) {
          SDL_SetWindowTitle(window, (const char *)editor0->file->name);
          editor = editor0;

        } else if(rect_contains(editor1->element.rect, e.button.x, e.button.y)) {
          SDL_SetWindowTitle(window, (const char *)editor1->file->name);
          editor = editor1;
        }
      }
    } else if(e.type == SDL_QUIT) {
      printf("Quitting application\n");
      break;
    }
  }

  element_destroy(editor0);
  element_destroy(editor1);
  backbuffer_destroy(platform.backbuffer);
  font_destroy(platform.font);

  return 0;
}
