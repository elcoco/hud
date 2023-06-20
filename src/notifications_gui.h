#ifndef NOTIFICATIONS_GUI_H
#define NOTIFICATIONS_GUI_H

#include <time.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "notifications.h"
#include "utils.h"
#include "module.h"


#define NOTIFICATION_TYPE_ITEM (notification_item_get_type ())
G_DECLARE_FINAL_TYPE(NotificationItem, notification_item, NOTIFICATION, ITEM, GObject)
struct _NotificationItem {
    GObject parent_instance;
    char *body;
    char *app;
    char *summary;
    time_t ts;
    GAppInfo *app_info;
};

#define NOTIFICATIONS_UI_PATH "/resources/ui/gui.ui"
#define NOTIFICATION_UI_PATH "/resources/ui/gui.ui"
#define NOTIFICATION_RESOURCE_DEFAULT_ICON "/resources/icons/notification.png"
#define NOTIFICATION_UPDATE_INTERVAL_MS 5000
#define NOTIFICATION_SEARCH_DELAY_MS 100

GObject* notifications_gui_init(struct Module *m);


#endif // !GUI_H
