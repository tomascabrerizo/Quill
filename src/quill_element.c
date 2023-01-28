#include "quill_element.h"

static void default_element_destroy(Element *element) {
  free(element);
}

Element *element_create(u64 size, Element *parent, MessageHandler default_messge_handler) {
  Element *element = (Element *)malloc(size);
  memset(element, 0, size);

  if(parent) {
    element->parent = parent;

    if(!parent->first_child) {
      parent->first_child = element;
    } else {
      parent->last_child->next = element;
    }
    parent->last_child = element;

  }

  element->default_message_handler = default_messge_handler;
  element->default_element_destroy = default_element_destroy;

  return element;
}

void element_destroy(Element *element) {
  Element *child = element->first_child;

  while(child) {
    Element *to_free = child;
    child = child->next;
    element_destroy(to_free);
  }

  if(element->user_element_destroy) {
    element->user_element_destroy(element);
  }

  element->default_element_destroy(element);
}

void element_set_user_message_handler(Element *element, MessageHandler user_message_handler) {
  element->user_message_handler = user_message_handler;
}

void element_set_user_element_destroy(Element *element, ElementDestroy user_element_destroy) {
  element->user_element_destroy = user_element_destroy;
}

void element_set_type(Element *element, ElementType type) {
  element->type = type;
}

static char *element_type_to_str[ELEMENT_TYPE_COUNT] = {
  "ELEMENT_NONE",
  "ELEMENT_LINE",
  "ELEMENT_FILE",
  "ELEMENT_EDITOR",
  "ELEMENT_APPLICATION",
};

char *element_get_type_str(Element *element) {
  return element_type_to_str[element->type];
}

