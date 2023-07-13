#ifndef MODULE_H
#define MODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <gtk/gtk.h>

#include "config.h"
#include "utils.h"


struct Module {
    GObject *widget;
    char *name;

    struct Config *config;

    struct Module *prev;
    struct Module *next;
    struct Module *head;

    GObject*(*init_cb)(struct Module*);
    void *args;

    _Atomic int lock;
    GObject *main_win;
};

struct Module* module_init(struct Module *m_prev, const char *name, struct Config *c, GObject*(*init_cb)(struct Module*), void* args);
void module_destroy(struct Module *m);
void module_destroy_all(struct Module *mod);

void module_activate(struct Module *m);
void module_deactivate(struct Module *m);
void module_debug(struct Module *m);

void module_lock(struct Module *m);
void module_unlock(struct Module *m);
int  module_is_locked(struct Module *m);
void module_set_main_win(struct Module *m, GObject *win);

#endif // !MODULE_H
