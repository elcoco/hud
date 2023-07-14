#ifndef APP_MODEL_H
#define APP_MODEL_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "utils.h"
#include "config.h"

#define APP_TYPE_ITEM (app_item_get_type ())
G_DECLARE_FINAL_TYPE(AppItem, app_item, APP, ITEM, GObject)
struct _AppItem {
    GObject parent_instance;

    GAppInfo *app_info;
};

void on_drag_begin (GtkDragSource *source, GdkDrag *drag, gpointer user_data);
GdkContentProvider *on_drag_prepare(GtkDragSource *source, double x, double y, gpointer user_data);
AppItem *app_item_new(GAppInfo *app_info);

int app_item_is_empty(AppItem *item);
void app_item_remove_empty(GListModel *model, int pos_exclude);
void app_item_write_config(GListModel *model, struct Config *c, const char *path);

#endif // !APP_MODEL_H
