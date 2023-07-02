#ifndef APP_MODEL_H
#define APP_MODEL_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#define APP_TYPE_ITEM (app_item_get_type ())
G_DECLARE_FINAL_TYPE(AppItem, app_item, APP, ITEM, GObject)
struct _AppItem {
    GObject parent_instance;

    GAppInfo *app_info;
};

GListModel *app_model_new(void);

#endif // !APP_MODEL_H
