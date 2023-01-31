#include "quill_element.h"
#include "quill_painter.h"
extern Platform platform;

static void default_element_destroy(Element *element) {
  free(element);
}

Element *element_create(u64 size, Element *parent, MessageHandler default_messge_handler) {
  Element *element = (Element *)malloc(size);
  memset(element, 0, size);

  if(parent) {
    element->parent = parent;
    element->backbuffer = parent->backbuffer;

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

void _element_destroy(Element *element) {
  Element *child = element->first_child;

  while(child) {
    Element *to_free = child;
    child = child->next;
    _element_destroy(to_free);
  }

  if(element->user_element_destroy) {
    element->user_element_destroy(element);
  }

  element->default_element_destroy(element);
}

u32 _element_get_width(Element *element) {
  if(rect_is_valid(element->rect)) {
    return (element->rect.r - element->rect.l);
  }
  return 0;
}

u32 _element_get_height(Element *element) {
  if(rect_is_valid(element->rect)) {
    return (element->rect.b - element->rect.t);
  }
  return 0;
}

u32 element_get_child_count(Element *element) {
  u32 result = 0;
  Element *child = element->first_child;
  while(child) {
    ++result;
    child = child->next;
  }
  return result;
}

void element_set_user_message_handler(Element *element, MessageHandler user_message_handler) {
  element->user_message_handler = user_message_handler;
}

void element_set_user_element_destroy(Element *element, ElementDestroy user_element_destroy) {
  element->user_element_destroy = user_element_destroy;
}

void element_set_backbuffer(Element *element, BackBuffer *backbuffer) {
  element->backbuffer = backbuffer;
  Element *child = element->first_child;
  while(child) {
    element_set_backbuffer(child, backbuffer);
    child = child->next;
  }
}

i32 _element_message(Element *element, Message message, void *data) {
  if(element->user_message_handler) {
    i32 result = element->user_message_handler(element, message, data);
    if(result) {
      return result;
    }
  }
  return element->default_message_handler(element, message, data);
}

void _element_draw(Element *element, Painter *painter) {
  Rect clip = rect_intersection(element->clip, painter->clipping);

  if(!rect_is_valid(clip)) {
    return;
  }

  painter->clipping = clip;
  _element_message(element, MESSAGE_DRAW, (void *)painter);

  Element *child = element->first_child;
  while(child) {
    painter->clipping = clip;
    _element_draw(child, painter);

    child = child->next;
  }
}

void _element_resize(Element *element, Rect rect) {
  Rect old_clip = element->clip;
  if(element->parent) {
    element->clip = rect_intersection(element->parent->clip, rect);
  } else {
    element->clip = rect;
  }

  if(!rect_equals(element->rect, rect) || !rect_equals(element->clip, old_clip)) {
    element->rect = rect;
    _element_message(element, MESSAGE_RESIZE, 0);
  }
}

void _element_redraw(Element *element, Rect *rect) {
  if(!rect) {
    rect = &element->rect;
  }
  Rect r = rect_intersection(*rect, element->clip);
  if(rect_is_valid(r)) {
    if(rect_is_valid(element->backbuffer->update_region)) {
      element->backbuffer->update_region = rect_union(element->backbuffer->update_region, r);
    } else {
      element->backbuffer->update_region = r;
    }
  }
  //rect_print(element->backbuffer->update_region);
}

void _element_update(Element *element) {
  Painter painter = painter_create(element->backbuffer);
  painter.font = platform.font;
  _element_draw(element, &painter);
  platform_end_draw(element->backbuffer);
  element->backbuffer->update_region = rect_create(0, 0, 0, 0);
}
