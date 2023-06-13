#include "notifications_gui.h"

//https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// to bind do window use: gtk_window_set_display (GTK_WINDOW (window),
//
//


#define NOTIFICATION_TYPE_ITEM (notification_item_get_type ())
G_DECLARE_FINAL_TYPE(NotificationItem, notification_item, NOTIFICATION, ITEM, GObject)
struct _NotificationItem {
    GObject parent_instance;
    const char *body;
    const char *app;
    const char *summary;
};

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

static NotificationItem *notification_item_new(const char *app, const char *body, const char *summary)
{
    NotificationItem *item;

    item = g_object_new(NOTIFICATION_TYPE_ITEM, NULL);

    item->app = app;
    item->body = body;
    item->summary = summary;

    return item;
}

static GListModel *notification_model_new(void)
{
    /* fill model with some fancy data */

    // get data
    struct NotifyItem *ni = notify_init(NULL);
    notify_req(100, ni);
    
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);

    while (ni != NULL) {

        NotificationItem *item = notification_item_new(ni->app, ni->summary, ni->body);
        g_list_store_append(store, item);
        ni = ni->next;
    }
    return G_LIST_MODEL(store);
}


static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Setup new rows */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_list_item_set_child(listitem, hbox);

    GtkWidget *lb_app = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(hbox), lb_app);

    GtkWidget *lb_body = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(hbox), lb_body);

    GtkWidget *lb_summary = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(hbox), lb_summary);

    GtkWidget *btn = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(btn), "Ok");
    gtk_box_append(GTK_BOX(hbox), btn);

    //gtk_list_item_set_child(listitem, lb);
    /* Because gtk_list_item_set_child sunk the floating reference of lb, releasing (unref) isn't necessary for lb. */
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    NotificationItem *item = gtk_list_item_get_item(listitem);

    GtkWidget *hbox = gtk_list_item_get_child(listitem);
    GtkWidget *lb_app = gtk_widget_get_first_child(GTK_WIDGET(hbox));
    GtkWidget *lb_body = gtk_widget_get_next_sibling(GTK_WIDGET(lb_app));
    GtkWidget *lb_summary = gtk_widget_get_next_sibling(GTK_WIDGET(lb_body));

    gtk_label_set_text(GTK_LABEL(lb_app), NOTIFICATION_ITEM(item)->app);
    gtk_label_set_text(GTK_LABEL(lb_body), NOTIFICATION_ITEM(item)->body);
    gtk_label_set_text(GTK_LABEL(lb_summary), NOTIFICATION_ITEM(item)->summary);
}

void str_to_lower(char *buf)
{
    for (int i=0 ; i<strlen(buf) ; i++)
        buf[i] = tolower(buf[i]);
}

int find_substr(const char *haystack, const char *needle)
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

gboolean custom_filter_cb(void* fitem, gpointer user_data)
{
    NotificationItem *item = NOTIFICATION_ITEM(fitem);
    GtkSearchEntry *w_search_entry = GTK_SEARCH_ENTRY(user_data);
    const char *inp_txt = gtk_editable_get_text(GTK_EDITABLE(w_search_entry));
    printf("search field: %s\n", inp_txt);

    if (strlen(inp_txt) == 0)
        return 1;

    // TODO quick fix for bug, for some reason there is a NULL at end of model, remove after fix
     if (item->app == NULL)
         return 0;

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

GObject* notifications_gui_init(GtkBuilder *builder, GObject *w_search_entry)
{
    // create our custom model
    GListModel *notification_model = notification_model_new();

    GObject *w_scroll_window = gtk_builder_get_object(builder, "scroll_window");
    GObject *w_list_view     = gtk_builder_get_object(builder, "list_view");

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

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_scroll_window;
}