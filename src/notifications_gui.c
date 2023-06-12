#include "notifications_gui.h"

//https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// to bind do window use: gtk_window_set_display (GTK_WINDOW (window),

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkWidget *lb = gtk_label_new(NULL);
    gtk_list_item_set_child(listitem, lb);
    /* Because gtk_list_item_set_child sunk the floating reference of lb, releasing (unref) isn't necessary for lb. */
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkWidget *lb = gtk_list_item_get_child(listitem);

    /* Strobj is owned by the instance. Caller mustn't change or destroy it. */
    GtkStringObject *strobj = gtk_list_item_get_item(listitem);

    /* The string returned by gtk_string_object_get_string is owned by the instance. */
    gtk_label_set_text(GTK_LABEL(lb), gtk_string_object_get_string(strobj));
}

GObject* notifications_gui_init(GtkBuilder *builder, GObject *w_search_entry)
{
    char *array[] = {
	"one", "two", "three", "four",
	"one", "two", "three", "four",
	"one", "two", "three", "four",
	"one", "two", "three", "four", NULL
    };

    GObject *w_scroll_window     = gtk_builder_get_object(builder, "scroll_window");
    GObject *w_list_view     = gtk_builder_get_object(builder, "list_view");

    // add filter to the string list model
    GtkStringList *string_list = gtk_string_list_new((const char *const *) array);
    GtkFilter *filter = GTK_FILTER(gtk_string_filter_new(gtk_property_expression_new(GTK_TYPE_STRING_OBJECT, NULL, "string")));
    GtkFilterListModel *filter_model = gtk_filter_list_model_new(G_LIST_MODEL(string_list), filter);
    gtk_filter_list_model_set_incremental(filter_model, TRUE);
    g_object_bind_property(w_search_entry, "text", filter, "search", 0);
    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(filter_model));


    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_scroll_window;
}
