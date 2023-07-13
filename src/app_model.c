#include "app_model.h"

struct _AppItemClass {
    GObjectClass parent_class;
};

G_DEFINE_TYPE(AppItem, app_item, G_TYPE_OBJECT)
static void app_item_init(AppItem *item)
{
}

static void app_item_class_init(AppItemClass *class)
{
}

AppItem *app_item_new(GAppInfo *app_info)
{
    AppItem *item;

    item = g_object_new(APP_TYPE_ITEM, NULL);
    item->app_info = app_info;

    //item->path = path;
    //item->name = name;
    //item->description = description;

    return item;
}

GdkContentProvider *on_drag_prepare(GtkDragSource *source, double x, double y, gpointer user_data)
{
    //GtkListItem *item = gtk_list_item_get_item(user_data);
    return gdk_content_provider_new_typed(G_TYPE_OBJECT, user_data);
}

void on_drag_begin(GtkDragSource *source, GdkDrag *drag, gpointer user_data)
{
    GtkListItem *listitem = GTK_LIST_ITEM(user_data);
    AppItem *item = gtk_list_item_get_item(listitem);
    GtkWidget *box = gtk_list_item_get_child(listitem);
    GtkWidget *image = gtk_widget_get_first_child(box);

    // Set the widget as the drag icon
    GdkPaintable *paintable = gtk_widget_paintable_new(GTK_WIDGET(image));
    gtk_drag_source_set_icon(source, paintable, 0, 0);
    g_object_unref(paintable);
    //printf("dragging start: %s\n", g_app_info_get_name(item->app_info));
}
