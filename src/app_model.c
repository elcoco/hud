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

static AppItem *app_item_new(GAppInfo *app_info)
{
    AppItem *item;

    item = g_object_new(APP_TYPE_ITEM, NULL);
    item->app_info = app_info;

    //item->path = path;
    //item->name = name;
    //item->description = description;

    return item;
}

GListModel *app_model_new(void)
{
    /* fill model with some fancy data */

    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    GList *apps = g_app_info_get_all();

    GList *app = apps;
    while (app != NULL) {
        GAppInfo *app_info = app->data;

        AppItem *item = app_item_new(app_info);
        g_list_store_append(store, item);

        app = app->next;
    }
    g_list_free(apps);
    return G_LIST_MODEL(store);
}
