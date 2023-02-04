#include "quill_application.h"
#include "quill_editor.h"
#include "quill_painter.h"

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

  } break;
  case MESSAGE_RESIZE: {

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
    element_message(application->current_editor, message, data);
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

Application *application_create(BackBuffer *backbuffer) {
  Application *application = (Application *)element_create(sizeof(Application), 0, application_default_message_handler);
  element_set_backbuffer(&application->element, backbuffer);
  return application;
}

void application_set_current_editor(Application *application, Editor *editor) {
  application->current_editor = editor;
}

