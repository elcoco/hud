#ifndef NOTIFICATIONS_GUI_H
#define NOTIFICATIONS_GUI_H

#include <time.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <pthread.h>

#include "notifications.h"
#include "utils.h"


#define NOTIFICATION_TYPE_ITEM (notification_item_get_type ())
G_DECLARE_FINAL_TYPE(NotificationItem, notification_item, NOTIFICATION, ITEM, GObject)
struct _NotificationItem {
    GObject parent_instance;
    const char *body;
    const char *app;
    const char *summary;
    time_t ts;
    GAppInfo *app_info;
};

struct ThreadState {
    int stopped;
    int(*cb)(void*);
    pthread_t id;
    void* arg;
    time_t interval_ms;
};

#define NOTIFICATIONS_UI_PATH "/resources/ui/gui.ui"
#define NOTIFICATION_UI_PATH "/resources/ui/gui.ui"
#define NOTIFICATION_RESOURCE_DEFAULT_ICON "/resources/icons/notification.png"

GObject* notifications_gui_init();


#endif // !GUI_H
