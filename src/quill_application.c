#include "quill_application.h"
#include "quill_data_structures.h"
#include "quill_editor.h"
#include "quill_painter.h"
#include "quill_file.h"

extern Platform platform;

static int application_default_message_handler(struct Element *element, Message message, void *data) {
  /* TODO: Implements default line message handler */
  (void)element; (void)message; (void)data;
  Application *application = (Application *)element;

  i32 gap = 10;

  switch(message) {
  case MESSAGE_DRAW: {
    Painter *painter = (Painter *)data;
    painter_draw_rect(painter, element->rect, 0x303030);
  } break;
  case MESSAGE_DRAW_ON_TOP: {
    Painter *painter = (Painter *)data;
    Application *application = (Application *)element;
    Rect rect = element_get_rect(application->current_editor);
    rect.l -= (gap/2);
    rect.r += (gap/2);
    rect.t -= (gap/2);
    rect.b += (gap/2);
    painter_draw_rect_outline(painter, rect, 0x0000ff);

    if(application->file_selector) {
      /* TODO: Try to manage the selector clipping always from the application */
      Rect old_clipping = painter->clipping;
      painter->clipping = application->file_selector_rect;
      painter_draw_rect(painter, application->file_selector_rect, 0x000000);
      Folder *folder = application->folder;
      if(folder) {
        i32 start_x = application->file_selector_rect.l;
        i32 start_y = application->file_selector_rect.t + platform.font->line_gap;
        for(u32 i = application->file_selector_offset; i < vector_size(folder->files); ++i) {
          File *file = folder->files[i];
          painter_draw_text(painter, file->name, strlen((char *)file->name), start_x, start_y, 0xffffff);
          start_y += platform.font->line_gap;
        }
        Rect selected_rect = application->file_selector_rect;
        selected_rect.t = application->file_selector_rect.t + ((application->file_selected_index - application->file_selector_offset)*platform.font->line_gap - platform.font->descender);
        selected_rect.b = selected_rect.t + platform.font->line_gap;
        painter_draw_rect_outline(painter, selected_rect, 0x0000ff);
      }
      painter->clipping = old_clipping;
    }

  } break;
  case MESSAGE_RESIZE: {

    /* NOTE: Resize file selector rect */
    application->file_selector_rect.l = (u32)(element_get_rect(application).r * 0.25f);
    application->file_selector_rect.r = (u32)(element_get_rect(application).r * 0.75f);
    application->file_selector_rect.t = (u32)(element_get_rect(application).b * 0.25f);
    application->file_selector_rect.b = (u32)(element_get_rect(application).b * 0.75f);

    /* NOTE: Resize editor of the application */
    i32 child_count = element_get_child_count(element);
    i32 rect_w = ((element->rect.r - element->rect.l) / child_count) - (gap + gap / 2);

    u32 i = 0;
    Element *child = element->first_child;
    while(child) {
      i32 l = i * rect_w + (gap * (i+1));
      i32 r = l + rect_w;
      i32 t = element->rect.t + gap;
      i32 b = element->rect.b - gap;
      _element_resize(child, rect_create(l, r, t, b));
      ++i;
      child = child->next;
    }

  } break;
  case MESSAGE_KEYDOWN: {
    u32 keycode = (u32)(u64)data;
    if(keycode == (EDITOR_KEY_P|EDITOR_MOD_CRTL)) {
      application->file_selector = !application->file_selector;
      Rect *rect = 0;
      if(application->file_selector) {
        rect = &application->file_selector_rect;
      }
      element_redraw(application, rect);
      element_update(application);
    }

    if(application->file_selector && application->folder && application->folder->files) {
      Rect *rect = 0;

      u32 selector_heihgt = application->file_selector_rect.b - application->file_selector_rect.t;
      u32 total_lines_view = selector_heihgt / platform.font->line_gap;

      if(keycode == EDITOR_KEY_DOWN) {
        application->file_selected_index = MIN(application->file_selected_index + 1, MAX(vector_size(application->folder->files)-1, 0));
        rect = &application->file_selector_rect;

        if(application->file_selected_index > (application->file_selector_offset + (total_lines_view - 1))) {
          application->file_selector_offset = application->file_selected_index - (total_lines_view - 1);
        }

      } else if(keycode == EDITOR_KEY_UP) {
        application->file_selected_index = MAX((i32)application->file_selected_index - 1, 0);
        rect = &application->file_selector_rect;

        if(application->file_selected_index < application->file_selector_offset) {
          application->file_selector_offset = application->file_selected_index;
        }

      } else if(keycode == EDITOR_KEY_ENTER) {
        if(application->current_editor->file) {
          application->current_editor->file->cursor_saved = application->current_editor->cursor;
        }
        File *file = application->folder->files[application->file_selected_index];
        application->current_editor->file = file;
        application->current_editor->cursor = file->cursor_saved;
        if(editor_should_scroll(application->current_editor)) {
          rect = 0;
        }

      }
      element_redraw(application, rect);
      element_update(application);

    } else {
      element_message(application->current_editor, message, data);
    }

  } break;
  case MESSAGE_KEYUP: {

  } break;
  case MESSAGE_TEXTINPUT: {
    element_message(application->current_editor, message, data);
  } break;
  case MESSAGE_BUTTONDOWN: {

    EditorMessage *message = (EditorMessage *)data;
    if(message->type == EDITOR_BUTTON_LEFT) {
      Application *application = (Application *)element;
      Element *child = element->first_child;
      Editor *old_current_editor = application->current_editor;
      while(child) {
        if(rect_contains(child->rect, message->button.x, message->button.y)) {
          /* TODO: Assert that the child is and editor */
          application->current_editor = (Editor *)child;
        }
        child = child->next;
      }
      if(old_current_editor != application->current_editor) {
        element_redraw(application, 0);
        element_update(application);
      }
    }

  } break;

  }

  return 0;
}

static void application_user_derstroy(Element *element) {
  Application *application = (Application *)element;
  folder_destroy(application->folder);
  printf("application destroy\n");
}

Application *application_create(BackBuffer *backbuffer) {
  Application *application = (Application *)element_create(sizeof(Application), 0, application_default_message_handler);

  element_set_backbuffer(&application->element, backbuffer);
  element_set_user_element_destroy(&application->element, application_user_derstroy);

  application->file_selector = false;
  application->file_selector_offset = 0;

  return application;
}

void application_set_current_editor(Application *application, Editor *editor) {
  application->current_editor = editor;
}

