#ifndef DASHBOARD_GUI_H
#define DASHBOARD_GUI_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "module.h"

#define DASHBOARD_UI_PATH "/resources/ui/gui.ui"

GObject* dashboard_gui_init(struct Module *m);


#endif // !DASHBOARD_GUI_H
