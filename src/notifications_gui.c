#include "notifications_gui.h"

//https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// to bind do window use: gtk_window_set_display (GTK_WINDOW (window),

struct _NotificationItemClass {
    GObjectClass parent_class;
};

G_DEFINE_TYPE(NotificationItem, notification_item, G_TYPE_OBJECT)
static void notification_item_init(NotificationItem *item)
{
}

static void notification_item_class_init(NotificationItemClass *class)
{
}

static NotificationItem *notification_item_new(const char *app, const char *body, const char *summary, time_t ts)
{
    NotificationItem *item;

    item = g_object_new(NOTIFICATION_TYPE_ITEM, NULL);

    item->app = strdup(app);
    item->body = strdup(body);
    item->summary = strdup(summary);
    item->ts = ts;
    item->app_info = find_appinfo(app);
    return item;
}

static GListModel *notification_model_new(void)
{
    /* fill model with some fancy data */
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    return G_LIST_MODEL(store);
}

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkBuilder *builder = gtk_builder_new_from_resource(NOTIFICATION_UI_PATH);
    GObject *w_box = gtk_builder_get_object(builder, "notification_box");
    gtk_list_item_set_child(listitem, GTK_WIDGET(w_box));
    //g_object_unref(builder);
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    NotificationItem *item = gtk_list_item_get_item(listitem);

    GtkWidget *main_box = gtk_list_item_get_child(listitem);
    GtkWidget *header_box = gtk_widget_get_first_child(GTK_WIDGET(main_box));
    
    GtkWidget *image = gtk_widget_get_first_child(GTK_WIDGET(header_box));
    GtkWidget *lb_app = gtk_widget_get_next_sibling(GTK_WIDGET(image));
    GtkWidget *lb_time = gtk_widget_get_next_sibling(GTK_WIDGET(lb_app));

    GtkWidget *lb_body = gtk_widget_get_next_sibling(GTK_WIDGET(header_box));
    GtkWidget *lb_summary = gtk_widget_get_next_sibling(GTK_WIDGET(lb_body));

    if (item->app_info != NULL)
        gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(item->app_info));
    else
        gtk_image_set_from_resource(GTK_IMAGE(image), NOTIFICATION_RESOURCE_DEFAULT_ICON);

    char app_buf[256] = "";
    char *app_escaped     = g_markup_escape_text(NOTIFICATION_ITEM(item)->app, -1);
    char *summary_escaped = g_markup_escape_text(NOTIFICATION_ITEM(item)->summary, -1);
    char *body_escaped    = g_markup_escape_text(NOTIFICATION_ITEM(item)->body, -1);

    sprintf(app_buf, "<b>%s</b>", app_escaped);
    gtk_label_set_markup(GTK_LABEL(lb_app), app_buf);
    gtk_label_set_markup(GTK_LABEL(lb_summary), summary_escaped);
    gtk_label_set_markup(GTK_LABEL(lb_body), body_escaped);

    free(app_escaped);
    free(summary_escaped);
    free(body_escaped);

    char ts[128] = "";
    ts_to_time_elapsed(NOTIFICATION_ITEM(item)->ts, ts);
    gtk_label_set_text(GTK_LABEL(lb_time), ts);
}

static void teardown_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    NotificationItem *item = gtk_list_item_get_item(listitem);
    if (item == NULL) {
        return;
    }
    printf("destroying item: %s\n", item->app);
    free(item->app);
    free(item->body);
    free(item->summary);
    g_object_unref(item->app_info);
    g_object_unref(item);

}

gboolean custom_filter_cb(void* fitem, gpointer user_data)
{
    NotificationItem *item = NOTIFICATION_ITEM(fitem);
    GtkSearchEntry *w_search_entry = GTK_SEARCH_ENTRY(user_data);
    const char *inp_txt = gtk_editable_get_text(GTK_EDITABLE(w_search_entry));

    if (strlen(inp_txt) == 0)
        return 1;

     // do the actual filtering
    if (find_substr(item->app, inp_txt) || find_substr(item->body, inp_txt) || find_substr(item->summary, inp_txt))
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

//gboolean keypressHandeler(GtkWidget *widget, GdkEventKey *event, gpointer data){
gboolean on_stop_search(GtkWidget *widget, gpointer data)
{
    printf("bevers!!!!!!\n");
    //gtk_widget_event(GTK_WIDGET(widget), event);
    return FALSE;

}

static void on_run_app_cb(GtkGridView *self, int pos, gpointer user_data)
{
    GListModel *model = G_LIST_MODEL(user_data);
    NotificationItem *item = g_list_model_get_item(G_LIST_MODEL(model), pos);
    printf("run app\n");
    if (item->app_info != NULL)
        g_app_info_launch(item->app_info, NULL, NULL, NULL);
    // get appinfo at model creation, can be used to start app and get icon
    //GAppInfo *app_info = item->app_info;

    // something something GError
    //g_app_info_launch(app_info, NULL, NULL, NULL);
}

int notifications_get_data_thread(void* arg)
{
    GListModel *model = arg;

    g_list_store_remove_all(G_LIST_STORE(model));

    struct NotifyItem *ni = notify_req(100);
    struct NotifyItem *first = ni;

    while (ni != NULL) {

        NotificationItem *item = notification_item_new(ni->app, ni->summary, ni->body, ni->ts);
        g_list_store_append(G_LIST_STORE(model), item);
        ni = ni->next;
    }
    if (first)
        notify_destroy(first);
    return 1;
}

GObject* notifications_gui_init(struct Module *m)
{


    // create our custom model
    GListModel *notification_model = notification_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(NOTIFICATIONS_UI_PATH);

    GObject *w_vbox          = gtk_builder_get_object(builder, "notifications_box");
    GObject *w_scroll_window = gtk_builder_get_object(builder, "notifications_sw");
    GObject *w_list_view     = gtk_builder_get_object(builder, "notifications_lv");
    GObject *w_search_entry  = gtk_builder_get_object(builder, "notifications_se");

    //g_object_unref(builder);

    // custom filter model that filters through all fields
    GtkFilter *filter = GTK_FILTER(gtk_custom_filter_new(custom_filter_cb, w_search_entry, NULL));
    GtkFilterListModel *filter_model = gtk_filter_list_model_new(G_LIST_MODEL(notification_model), filter);
    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(filter_model));

    // connect search input via callback to update model on change
    g_signal_connect(G_OBJECT(w_search_entry), "search-changed", G_CALLBACK(search_entry_changed_cb), filter);

    // factory creates widgets to connect model to view
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);
    g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);

    g_signal_connect(w_search_entry, "stop-search",  G_CALLBACK(on_stop_search), NULL);
    g_signal_connect(G_OBJECT(w_list_view), "activate", G_CALLBACK(on_run_app_cb), no_sel);

    // find signal id for type/signal
    guint sig_id;
    g_signal_parse_name("stop-search", GTK_TYPE_SEARCH_ENTRY, &sig_id, NULL, 0);

    // find signal handler for instance with matching signal id
    //gulong sig_handler = g_signal_handler_find(G_OBJECT(w_search_entry), G_SIGNAL_MATCH_ID, sig_id, 0, NULL, NULL, NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    // Run function without blocking main thread
    notifications_get_data_thread(notification_model);
    g_timeout_add(NOTIFICATION_UPDATE_INTERVAL_MS, notifications_get_data_thread, notification_model);

    return w_vbox;
}

