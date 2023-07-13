#ifndef APPS_H
#define APPS_H

#include <ctype.h>      // to_lower()
                        //
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "module.h"
#include "app_model.h"
#include "utils.h"



#define APPS_UI_PATH "/resources/ui/gui.ui"

GObject* apps_gui_init(struct Module *m);

#endif // !APPS_H
