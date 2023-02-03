#ifndef _QUILL_ELEMENT_H_
#define _QUILL_ELEMENT_H_

#include "quill.h"

typedef enum Message {
  MESSAGE_RESIZE,
  MESSAGE_DRAW,
  MESSAGE_KEYDOWN,
  MESSAGE_KEYUP,
  MESSAGE_TEXTINPUT,
  MESSAGE_BUTTONDOWN,
} Message;

struct Element;
typedef i32 (*MessageHandler)(struct Element *element, Message message, void *data);
typedef void (*ElementDestroy)(struct Element *element);

struct Painter;

typedef struct Element {
  Rect rect, clip;

  struct Element *parent;
  struct Element *first_child;
  struct Element *last_child;
  struct Element *next;

  MessageHandler default_message_handler;
  MessageHandler user_message_handler;

  ElementDestroy default_element_destroy;
  ElementDestroy user_element_destroy;

  BackBuffer *backbuffer;
} Element;

Element *element_create(u64 size, Element *parent, MessageHandler default_messge_handler);
#define element_destroy(e) _element_destroy(&((e)->element))
void _element_destroy(Element *element);

#define element_get_rect(e) ((e)->element.rect)
#define element_get_width(e) _element_get_width(&(e)->element)
#define element_get_height(e) _element_get_height(&(e)->element)
u32 _element_get_width(Element *element);
u32 _element_get_height(Element *element);
u32 element_get_child_count(Element *element);

void element_set_user_message_handler(Element *element, MessageHandler user_message_handler);
void element_set_user_element_destroy(Element *element, ElementDestroy user_element_destroy);
void element_set_backbuffer(Element *element, BackBuffer *backbuffer);

#define element_message(e, message, data) _element_message(&(e)->element, (message), (void *)((u64)(data)))
#define element_draw(e, painter) _element_draw(&(e)->element, (painter))
#define element_resize(e, rect) _element_resize(&(e)->element, (rect))
#define element_redraw(e, rect_ptr) _element_redraw(&(e)->element, (rect_ptr))
#define element_update(e) _element_update(&(e)->element)

i32 _element_message(Element *element, Message message, void *data);
void _element_draw(Element *element, struct Painter *painter);
void _element_resize(Element *element, Rect rect);
void _element_redraw(Element *element, Rect *rect);
void _element_update(Element *element);

#define QUILL_ELEMENT Element element;

#endif /* _QUILL_ELEMENT_H_ */
