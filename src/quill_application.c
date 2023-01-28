#include "quill_application.h"

static int application_default_message_handler(struct Element *element, Message message, void *data) {
  /* TODO: Implements default line message handler */
  (void)element; (void)message; (void)data;
  return 0;
}

Application *application_create(BackBuffer *backbuffer) {
  Application *application = (Application *)element_create(sizeof(Application), 0, application_default_message_handler);
  application->backbuffer = backbuffer;
  return application;
}

void application_destroy(Application *application) {
  element_destroy((Element *)application);
}
