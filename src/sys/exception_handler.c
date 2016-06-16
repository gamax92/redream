#include "core/assert.h"
#include "core/list.h"
#include "sys/exception_handler.h"

typedef struct exc_handler_s {
  void *data;
  exception_handler_cb cb;
  list_node_t it;
} exc_handler_t;

static const int MAX_EXCEPTION_HANDLERS = 32;
static exc_handler_t s_handlers[MAX_EXCEPTION_HANDLERS];
static list_t s_live_handlers;
static list_t s_free_handlers;
static int s_num_handlers;

bool exception_handler_install() {
  for (int i = 0; i < MAX_EXCEPTION_HANDLERS; i++) {
    exc_handler_t *handler = &s_handlers[i];
    list_add(&s_free_handlers, &handler->it);
  }

  return exception_handler_install_platform();
}

void exception_handler_uninstall() {
  exception_handler_uninstall_platform();
}

exc_handler_t *exception_handler_add(void *data, exception_handler_cb cb) {
  // remove from free list
  exc_handler_t *handler =
      list_first_entry(&s_free_handlers, exc_handler_t, it);
  CHECK_NOTNULL(handler);
  list_remove(&s_free_handlers, &handler->it);

  // add to live list
  handler->data = data;
  handler->cb = cb;
  list_add(&s_live_handlers, &handler->it);

  return handler;
}

void exception_handler_remove(exc_handler_t *handler) {
  list_remove(&s_live_handlers, &handler->it);
  list_add(&s_free_handlers, &handler->it);
}

bool exception_handler_handle(re_exception_t *ex) {
  list_for_each_entry(handler, &s_live_handlers, exc_handler_t, it) {
    if (handler->cb(handler->data, ex)) {
      return true;
    }
  }

  return false;
}