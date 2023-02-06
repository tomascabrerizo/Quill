#ifndef _QUILL_APPLICATION_H_
#define _QUILL_APPLICATION_H_

#include "quill.h"
#include "quill_element.h"
#include "quill_cursor.h"

struct Editor;
struct Folder;

typedef struct Application {
  QUILL_ELEMENT

  /* TODO: The application should have and array of Editors to easy access them */

  bool file_selector;
  u32 file_selected_index;
  u32 file_selector_offset;
  Rect file_selector_rect;

  struct Editor *current_editor;
  struct Folder *folder;

} Application;

Application *application_create(BackBuffer *backbuffer);
void application_set_current_editor(Application *application, struct Editor *editor);

#endif /* _QUILL_APPLICATION_H_ */
