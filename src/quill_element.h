#ifndef _QUILL_ELEMENT_H_
#define _QUILL_ELEMENT_H_

#include "quill.h"

/* TODO: Maybe Message should be a discriminated union */

typedef enum Message {
  MESSAGE_RESIZE,
  MESSAGE_DRAW,
  MESSAGE_KEYDOWN,
  MESSAGE_KEYUP
} Message;

struct Element;
typedef int (*MessageHandler)(struct Element *element, Message message, void *data);
typedef void (*ElementDestroy)(struct Element *element);

typedef enum ElementType {
  ELEMENT_NONE,
  ELEMENT_LINE,
  ELEMENT_FILE,
  ELEMENT_EDITOR,
  ELEMENT_APPLICATION,
  ELEMENT_TYPE_COUNT
} ElementType;

typedef struct Element {
  ElementType type;
  Rect rect;

  struct Element *parent;
  struct Element *first_child;
  struct Element *last_child;
  struct Element *next;

  MessageHandler default_message_handler;
  MessageHandler user_message_handler;

  ElementDestroy default_element_destroy;
  ElementDestroy user_element_destroy;

} Element;

Element *element_create(u64 size, Element *parent, MessageHandler default_messge_handler);
void element_destroy(Element *element);
void element_set_user_message_handler(Element *element, MessageHandler user_message_handler);
void element_set_user_element_destroy(Element *element, ElementDestroy user_element_destroy);
void element_set_type(Element *element, ElementType type);
char *element_get_type_str(Element *element);

#define QUILL_ELEMENT Element element;

#endif /* _QUILL_ELEMENT_H_ */
