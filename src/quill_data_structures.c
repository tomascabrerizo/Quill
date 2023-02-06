#include "quill_data_structures.h"

void *vector_grow(void *vector, u32 element_size) {
  if(vector == 0) {
    VectorHeader *header = (VectorHeader *)malloc(sizeof(VectorHeader) + element_size * VECTOR_DEFAULT_CAPACITY);
    header->capacity = VECTOR_DEFAULT_CAPACITY;
    header->size = 0;
    return (void *)(header + 1);
  } else {
    VectorHeader *header = vector_header(vector);
    u32 new_vector_size = header->capacity * 2;
    header = (VectorHeader *)realloc(header, sizeof(VectorHeader) + new_vector_size * element_size);
    header->capacity = new_vector_size;
    return (void *)(header + 1);
  }
}

void *gapbuffer_grow(void *buffer, u32 element_size) {
  if(buffer == 0) {
    GapBufferHeader *header = (GapBufferHeader *)malloc(sizeof(GapBufferHeader) + element_size * GAPBUFFER_DEFAULT_CAPACITY);
    header->capacity = GAPBUFFER_DEFAULT_CAPACITY;
    header->f_index = 0;
    header->s_index = GAPBUFFER_DEFAULT_CAPACITY;
    return (void *)(header + 1);
  } else {
    GapBufferHeader *header = gapbuffer_header(buffer);
    u32 new_buffer_size = header->capacity * 2;
    header = (GapBufferHeader *)realloc(header, sizeof(GapBufferHeader) + new_buffer_size * element_size);
    u32 second_gap_size = header->capacity - header->s_index;
    void *des = (u8 *)(header + 1) + (new_buffer_size - second_gap_size) * element_size;
    void *src = (u8 *)(header + 1) + (header->capacity - second_gap_size) * element_size;
    memmove(des, src, second_gap_size * element_size);
    header->s_index = new_buffer_size - second_gap_size;
    header->capacity = new_buffer_size;
    return (header + 1);
  }
}
