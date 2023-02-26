#ifndef _QUILL_DATA_STRUCTURE_H_
#define _QUILL_DATA_STRUCTURE_H_

#include "quill.h"

/* Quill Data structures */

#define QUILL_DLL(typename) struct typename *next, *prev;

/* NOTE: This is a Circular double link list and need to have a dummy object setup, pease dont forget to call dll_init */
#define dll_init(node) ((node)->prev = (node), (node)->next = (node))

#define dll_insert_front(node0, node1) \
  ((node0)->next = (node1), (node0)->prev = (node1)->prev, \
  (node0)->prev->next = (node0), (node0)->next->prev = (node0))

#define dll_insert_back(node0, node1) \
  ((node0)->prev = (node1), (node0)->next = (node1)->next, \
  (node0)->prev->next = (node0), (node0)->next->prev = (node0))

#define VECTOR_DEFAULT_CAPACITY 8

typedef struct VectorHeader {
  u32 capacity;
  u32 size;
} VectorHeader;

void *vector_grow(void *vector, u32 element_size);

#define vector_header(vector) ((VectorHeader *)((u8 *)(vector) - sizeof(VectorHeader)))

#define vector_capacity(vector) ((vector) != 0 ? vector_header((vector))->capacity : 0)

#define vector_size(vector) ((vector) != 0 ? vector_header((vector))->size : 0)

#define vector_clear(vector) ((vector) != 0 ? (vector_header((vector))->size = 0) : 0)

#define vector_fit(vector) (vector_size((vector)) >= vector_capacity((vector)) ? \
  (vector) = vector_grow((vector), sizeof(*(vector))) : 0)

#define vector_push(vector, value) (vector_fit(vector), (vector)[vector_header((vector))->size++] = (value))

#define vector_free(vector) ((vector != 0) ? (free(vector_header((vector)))) : 0)

#define GAPBUFFER_DEFAULT_CAPACITY 8

typedef struct GapBufferHeader {
  u32 capacity;
  u32 f_index;
  u32 s_index;
} GapBufferHeader;

void *gapbuffer_grow(void *buffer, u32 element_size);

#define gapbuffer_header(buffer) ((GapBufferHeader *)((u8 *)(buffer) - sizeof(GapBufferHeader)))

#define gapbuffer_capacity(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->capacity : 0)

#define gapbuffer_f_index(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->f_index : 0)

#define gapbuffer_s_index(buffer) ((buffer) != 0 ? gapbuffer_header((buffer))->s_index : 1)

#define gapbuffer_size(buffer) ((buffer) != 0 ? \
  (gapbuffer_f_index((buffer)) + gapbuffer_capacity((buffer)) - gapbuffer_s_index((buffer))) : 0)

#define gapbuffer_fit(buffer) ((gapbuffer_f_index((buffer)) == (gapbuffer_s_index((buffer))-1)) ? \
  (buffer) = gapbuffer_grow((buffer), sizeof(*(buffer))) : 0)

#define gapbuffer_step_foward(buffer) (gapbuffer_s_index((buffer)) < gapbuffer_capacity((buffer)) ? \
  ((buffer)[gapbuffer_f_index((buffer))] = (buffer)[gapbuffer_s_index((buffer))], \
  gapbuffer_header((buffer))->f_index++, gapbuffer_header((buffer))->s_index++) : 0)

#define gapbuffer_step_backward(buffer) (gapbuffer_f_index((buffer)) > 0 ? \
  ((buffer)[gapbuffer_s_index((buffer)) - 1] = (buffer)[gapbuffer_f_index((buffer)) - 1], \
  gapbuffer_header((buffer))->f_index--, gapbuffer_header((buffer))->s_index--) : 0)

#define gapbuffer_insert(buffer, value) (gapbuffer_fit((buffer)), \
  (buffer)[gapbuffer_f_index((buffer))] = (value), gapbuffer_header((buffer))->f_index++)

#define gapbuffer_remove(buffer) (gapbuffer_f_index((buffer)) > 0 ? \
  gapbuffer_header((buffer))->f_index-- : 0)

#define gapbuffer_free(buffer) ((buffer != 0) ? (free(gapbuffer_header((buffer)))) : 0)

#define gapbuffer_get_at_gap(buffer) ((buffer)[gapbuffer_f_index((buffer))-1])

#define gapbuffer_at(buffer, index) \
  (((index) > gapbuffer_f_index((buffer))) ? \
  (buffer)[index + (gapbuffer_s_index((buffer)) - gapbuffer_f_index((buffer)))] : \
  (buffer)[index])

#define gapbuffer_move_to(buffer, index) \
  do {\
  if(index != gapbuffer_f_index(buffer)) { \
  assert(index < gapbuffer_capacity(buffer)); \
  i32 distance = (i32)index - (i32)gapbuffer_f_index(buffer); \
  if(distance > 0) { \
  for(u32 i = gapbuffer_f_index(buffer); i < index; ++i) { \
  gapbuffer_step_foward(buffer); \
  } \
  } else if(distance < 0) { \
  for(u32 i = gapbuffer_f_index(buffer); i > index; --i) { \
  gapbuffer_step_backward(buffer); \
  } \
  } \
  } \
  } while(0)

#endif /* _QUILL_DATA_STRUCTURE_H_ */

