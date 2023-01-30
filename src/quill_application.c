#include "quill_application.h"
#include "quill_editor.h"
#include "quill_painter.h"

extern Platform platform;

static int application_default_message_handler(struct Element *element, Message message, void *data) {
  /* TODO: Implements default line message handler */
  (void)element; (void)message; (void)data;
  Application *application = (Application *)element;

  switch(message) {
  case MESSAGE_DRAW: {
    Painter *painter = (Painter *)data;
    painter_draw_rect(painter, element->rect, 0x202020);
  } break;
  case MESSAGE_RESIZE: {

    i32 child_count = element_get_child_count(element);
    i32 rect_w = (element->rect.r - element->rect.l) / child_count;

    u32 i = 0;
    Element *child = element->first_child;
    while(child) {
      i32 l = i * rect_w;
      i32 r = l + rect_w;
      i32 t = element->rect.t;
      i32 b = element->rect.b;
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

