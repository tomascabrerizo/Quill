#ifndef _QUILL_APPLICATION_H_
#define _QUILL_APPLICATION_H_

#include "quill.h"
#include "quill_element.h"

struct Editor;

typedef struct Application {
  QUILL_ELEMENT
  struct Editor *current_editor;
} Application;

Application *application_create(BackBuffer *backbuffer);
void application_set_current_editor(Application *application, struct Editor *editor);
void application_update(Application *application);

#endif /* _QUILL_APPLICATION_H_ */
