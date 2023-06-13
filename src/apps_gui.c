#include "apps_gui.h"

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

static GListModel *app_model_new(void)
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
    return G_LIST_MODEL(store);
}


static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Setup new rows */

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_list_item_set_child(listitem, vbox);

    GtkWidget *image = gtk_image_new ();
    gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_LARGE);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 100);
    gtk_box_append(GTK_BOX(vbox), image);

    GtkWidget *lb_name = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(vbox), lb_name);
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    AppItem *item = gtk_list_item_get_item(listitem);

    GtkWidget *vbox = gtk_list_item_get_child(listitem);
    GtkWidget *image = gtk_widget_get_first_child(GTK_WIDGET(vbox));
    GtkWidget *lb_name = gtk_widget_get_next_sibling(GTK_WIDGET(image));


    gtk_box_append(GTK_BOX(vbox), image);

    gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(APP_ITEM(item)->app_info));
    gtk_label_set_text(GTK_LABEL(lb_name), g_app_info_get_name(APP_ITEM(item)->app_info));
}

static void str_to_lower(char *buf)
{
    for (int i=0 ; i<strlen(buf) ; i++)
        buf[i] = tolower(buf[i]);
}

static int find_substr(const char *haystack, const char *needle)
{
    char *str = strdup(haystack);
    char *substr = strdup(needle);
    str_to_lower(str);
    str_to_lower(substr);

    int res = (strstr(str, substr)) ? 1 : 0;

    free(str);
    free(substr);

    return res;
}

static gboolean custom_filter_cb(void* fitem, gpointer user_data)
{
    AppItem *item = APP_ITEM(fitem);
    GtkSearchEntry *w_search_entry = GTK_SEARCH_ENTRY(user_data);
    const char *inp_txt = gtk_editable_get_text(GTK_EDITABLE(w_search_entry));

    if (strlen(inp_txt) == 0)
        return 1;

     // do the actual filtering
    if (find_substr(g_app_info_get_name(APP_ITEM(item)->app_info), inp_txt))
        return 1;
    else
        return 0;
}

static void search_entry_changed_cb(void* self, gpointer user_data)
{
    /* Is called when search entry is changed and notifies users of filter to update the models */
    GtkFilter *filter = GTK_FILTER(user_data);
    gtk_filter_changed(filter, GTK_FILTER_CHANGE_DIFFERENT);
}

GObject* apps_gui_init(GObject *w_search_entry)
{
    // create our custom model
    GListModel *app_model = app_model_new();

    GtkBuilder *builder = gtk_builder_new_from_file(APPS_UI_PATH);

    GObject *w_scroll_window = gtk_builder_get_object(builder, "sw_apps");
    GObject *w_grid_view     = gtk_builder_get_object(builder, "gv_apps");

    // custom filter model that filters through all fields
    GtkFilter *filter = GTK_FILTER(gtk_custom_filter_new(custom_filter_cb, w_search_entry, NULL));
    GtkFilterListModel *filter_model = gtk_filter_list_model_new(G_LIST_MODEL(app_model), filter);
    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(filter_model));

    // connect search input via callback to update model on change
    g_signal_connect(G_OBJECT(w_search_entry), "search-changed", G_CALLBACK(search_entry_changed_cb), filter);

    // factory creates widgets to connect model to view
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);

    //GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(app_model));
    //gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    //gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_grid_view_set_model(GTK_GRID_VIEW(w_grid_view), GTK_SELECTION_MODEL(no_sel));
    gtk_grid_view_set_factory(GTK_GRID_VIEW(w_grid_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_grid_view));
    //gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_scroll_window;
}
