#ifndef NOTIFICATIONS_GUI_H
#define NOTIFICATIONS_GUI_H

#include <ctype.h>      // to_lower()

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "notifications.h"


#define NOTIFICATION_TYPE_ITEM (notification_item_get_type ())
G_DECLARE_FINAL_TYPE(NotificationItem, notification_item, NOTIFICATION, ITEM, GObject)
struct _NotificationItem {
    GObject parent_instance;
    const char *body;
    const char *app;
    const char *summary;
};

#define APPS_UI_PATH "src/gui/gui.ui"

GObject* notifications_gui_init();


#endif // !GUI_H
