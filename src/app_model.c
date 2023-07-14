#include "app_model.h"
#include "json.h"

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
    GtkWidget *box = gtk_list_item_get_child(listitem);
    GtkWidget *image = gtk_widget_get_first_child(box);

    // Set the widget as the drag icon
    GdkPaintable *paintable = gtk_widget_paintable_new(GTK_WIDGET(image));
    gtk_drag_source_set_icon(source, paintable, 0, 0);
    g_object_unref(paintable);
    //printf("dragging start: %s\n", g_app_info_get_name(item->app_info));
}

int app_item_is_empty(AppItem *item)
{
    /* check if this is a placeholder, used for ordering a dock */
    return item->app_info == NULL;
}


void app_item_remove_empty(GListModel *model, int pos_exclude)
{
    /* remove all empty items except for item at pos_exclude
     * if pos == -1, don't exclude anything
     */

    for (int i=g_list_model_get_n_items(model)-1 ; i>=0 ; i--) {
        if (pos_exclude >= 0 && i == pos_exclude)
            continue;

        AppItem *item = APP_ITEM(g_list_model_get_object(model, i));
        if (app_item_is_empty(item)) {
            DEBUG("removing empty: %d\n", i);
            g_list_store_remove(G_LIST_STORE(model), i);
        }
    }
}

void app_item_write_config(GListModel *model, struct Config *c, const char *path)
{
    struct JSONObject *rn = json_load_file(c->path);
    if (rn == NULL)
        return;
    
    struct JSONObject *apps = json_get_path(rn, path);
    if (apps == NULL)
        return;

    // remove old children
    struct JSONObject *app = apps->value;
    while (app != NULL) {
        struct JSONObject *tmp = app;
        app = app->next;
        json_object_destroy(tmp);
    }

    // add new children
    for (int i=0 ; i < g_list_model_get_n_items(model) ; i++) {
        AppItem *item = APP_ITEM(g_list_model_get_object(model, i));
        if (app_item_is_empty(item))
            continue;

        struct JSONObject *app = json_object_init_object(NULL, NULL);
        json_object_init_string(app, "type", "appinfo");
        json_object_init_string(app, "name", g_app_info_get_name(item->app_info));
        json_object_append_child(apps, app);
    }
    json_object_to_file(rn, c->path, 4);
}
