#ifndef _QUILL_APPLICATION_H_
#define _QUILL_APPLICATION_H_

#include "quill.h"
#include "quill_element.h"

typedef struct Application {
  QUILL_ELEMENT

  BackBuffer *backbuffer;
} Application;

Application *application_create(BackBuffer *backbuffer);
void application_destroy(Application *application);

#endif /* _QUILL_APPLICATION_H_ */
