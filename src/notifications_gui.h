#ifndef NOTIFICATIONS_GUI_H
#define NOTIFICATIONS_GUI_H

#include <ctype.h>      // to_lower()

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "notifications.h"

GObject* notifications_gui_init(GtkBuilder *builder, GObject *w_search_entry);

#endif // !GUI_H
