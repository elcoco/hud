#include "gui.h"

//https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// to bind do window use: gtk_window_set_display (GTK_WINDOW (window),
                              gtk_widget_get_display (do_widget));

static void bevers(GtkWindow *window)
{
    gtk_window_close(window);
}

static void gui_activate2(GtkApplication *app, gpointer user_data)
{
    /* Construct a GtkBuilder instance and load our UI description */
    //GtkBuilder *builder = gtk_builder_new ();
    //gtk_builder_add_from_file (builder, "gui.ui", NULL);
    GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");

    /* Connect signal handlers to the constructed widgets. */
    GtkWidget *window = gtk_builder_get_object(builder, "win");
    gtk_window_set_application(GTK_WINDOW(window), app);

    gtk_window_present(GTK_WINDOW(window));

    /* We do not need the builder any more */
    g_object_unref(builder);
}

int start_gui(int argc, char *argv[])
{

    GtkApplication *app = gtk_application_new("org.gtk.example",
					      G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(gui_activate2), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}


void gui_on_exit(GtkWindow *win, gpointer user_data)
{
    gtk_window_close(win);
}


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

static void app_activate(GtkApplication *app)
{
    char *array[] = {
	"one", "two", "three", "four",
	"one", "two", "three", "four",
	"one", "two", "three", "four",
	"one", "two", "three", "four", NULL
    };


    GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");



    GObject *window = gtk_builder_get_object(builder, "win");
    GObject *w_scroll_window     = gtk_builder_get_object(builder, "scroll_window");
    GObject *w_list_view     = gtk_builder_get_object(builder, "list_view");
    GObject *w_search_entry = gtk_builder_get_object(builder, "search_entry");


    //g_signal_connect(w_search_entry, "search-changed", G_CALLBACK(search_text_changed), NULL);

    //GtkListItemFactory *factory = GTK_LIST_ITEM_FACTORY(gtk_builder_get_object(builder, "factory"));

    g_signal_connect(window, "close-request", G_CALLBACK(gui_on_exit), NULL);
    gtk_window_set_application(GTK_WINDOW(window), app);


    // add filter to the string list model
    GtkStringList *string_list = gtk_string_list_new((const char *const *) array);
    GtkFilter *filter = GTK_FILTER(gtk_string_filter_new(gtk_property_expression_new(GTK_TYPE_STRING_OBJECT, NULL, "string")));
    GtkFilterListModel *filter_model = gtk_filter_list_model_new(G_LIST_MODEL(string_list), filter);
    gtk_filter_list_model_set_incremental(filter_model, TRUE);
    g_object_bind_property (w_search_entry, "text", filter, "search", 0);
    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(filter_model));


    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));



    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));



    gtk_window_present(GTK_WINDOW(window));

    /* We do not need the builder any more */
    g_object_unref(builder);


    /* sl is owned by ns */
    /* ns and factory are owned by lv. */
    /* Therefore, you don't need to care about their destruction. */
    /*
    GtkStringList *sl = gtk_string_list_new((const char *const *) array);
    GtkNoSelection *ns = gtk_no_selection_new(G_LIST_MODEL(sl));

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_cb), NULL);
    g_signal_connect(factory, "unbind", G_CALLBACK(unbind_cb), NULL);
    g_signal_connect(factory, "teardown", G_CALLBACK(teardown_cb), NULL);

    GtkWidget *lv = gtk_list_view_new(GTK_SELECTION_MODEL(ns), factory);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scr), lv);
    gtk_window_present(GTK_WINDOW(win));
    */
}

int show_gui(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("com.bever.eco", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    int stat = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return stat;
}
