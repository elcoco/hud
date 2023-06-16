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

    return item;
}

static GListModel *notification_model_new(void)
{
    /* fill model with some fancy data */

    // get data
    //struct NotifyItem *ni = notify_init(NULL);
    struct NotifyItem *ni = notify_req(100);
    
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);

    while (ni != NULL) {

        NotificationItem *item = notification_item_new(ni->app, ni->summary, ni->body, ni->ts);
        g_list_store_append(store, item);
        ni = ni->next;
    }
    // TODO destroy ni
    return G_LIST_MODEL(store);
}


static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkBuilder *builder = gtk_builder_new_from_resource(NOTIFICATION_UI_PATH);

    GObject *w_box = gtk_builder_get_object(builder, "notification_box");
    GObject *w_icon = gtk_builder_get_object(builder, "notification_icon");
    GObject *w_app_lb = gtk_builder_get_object(builder, "notification_app_lb");
    GObject *w_time_lb = gtk_builder_get_object(builder, "notification_time_lb");

    GObject *w_summ_lb = gtk_builder_get_object(builder, "notification_summary_lb");
    GObject *w_body_lb = gtk_builder_get_object(builder, "notification_body_lb");

    gtk_list_item_set_child(listitem, GTK_WIDGET(w_box));

    /* Setup new rows */
    //GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    //gtk_list_item_set_child(listitem, hbox);

    //GtkWidget *lb_app = gtk_label_new(NULL);
    //gtk_box_append(GTK_BOX(hbox), lb_app);

    //GtkWidget *lb_body = gtk_label_new(NULL);
    //gtk_box_append(GTK_BOX(hbox), lb_body);

    //GtkWidget *lb_summary = gtk_label_new(NULL);
    //gtk_box_append(GTK_BOX(hbox), lb_summary);
}

static GIcon* find_icon(const char *app_name)
{
    // lookup icon
    GList *apps = g_app_info_get_all();
    GList *app = apps;

    GIcon *match = NULL;
    while (app != NULL) {
        GAppInfo *app_info = app->data;
        const char *name = g_app_info_get_name(app_info);

        if (strcmp(name, app_name) == 0) {
            match = g_app_info_get_icon(app_info);
            break;
        }
        app = app->next;
    }
    return match;
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

    GtkWidget *lb_summary = gtk_widget_get_next_sibling(GTK_WIDGET(header_box));
    GtkWidget *lb_body = gtk_widget_get_next_sibling(GTK_WIDGET(lb_summary));

    GIcon *icon = find_icon(NOTIFICATION_ITEM(item)->app);
    if (icon == NULL)
        gtk_image_set_from_resource(GTK_IMAGE(image), NOTIFICATION_RESOURCE_DEFAULT_ICON);
    else
        gtk_image_set_from_gicon(GTK_IMAGE(image), icon);

    gtk_label_set_text(GTK_LABEL(lb_app), NOTIFICATION_ITEM(item)->app);
    gtk_label_set_text(GTK_LABEL(lb_body), NOTIFICATION_ITEM(item)->body);
    gtk_label_set_text(GTK_LABEL(lb_summary), NOTIFICATION_ITEM(item)->summary);

    char ts[128] = "";
    strftime(ts, 128, "%Y-%m-%d %H:%M:%S", localtime(&NOTIFICATION_ITEM(item)->ts));
    gtk_label_set_text(GTK_LABEL(lb_time), ts);
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

GObject* notifications_gui_init()
{
    /*
    static int is_initialized = 0;
    if (is_initialized)
        return NULL;
    is_initialized = 1;
    */

    // create our custom model
    GListModel *notification_model = notification_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(NOTIFICATIONS_UI_PATH);

    GObject *w_vbox          = gtk_builder_get_object(builder, "notifications_box");
    GObject *w_scroll_window = gtk_builder_get_object(builder, "notifications_sw");
    GObject *w_list_view     = gtk_builder_get_object(builder, "notifications_lv");
    GObject *w_search_entry  = gtk_builder_get_object(builder, "notifications_se");

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

    g_signal_connect(w_search_entry, "stop-search",  G_CALLBACK(on_stop_search), NULL);

    // find signal id for type/signal
    guint sig_id;
    g_signal_parse_name("stop-search", GTK_TYPE_SEARCH_ENTRY, &sig_id, NULL, 0);
    // find signal handler for instance with matching signal id
    gulong sig_handler = g_signal_handler_find(G_OBJECT(w_search_entry), G_SIGNAL_MATCH_ID, sig_id, 0, NULL, NULL, NULL);
    // disconnect handler
    //g_signal_handler_disconnect(G_OBJECT(w_search_entry), sig_handler);

    printf("FOUND ID: %d\n", sig_id);
    printf("FOUND HANDLER: %ld\n", sig_handler);


    //GtkEventController *controller = gtk_shortcut_controller_new();
    //gtk_widget_add_controller(GTK_WIDGET(w_search_entry), controller);
    //gtk_shortcut_controller_remove_shortcut(controller, shortcut);





    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_vbox;
}

