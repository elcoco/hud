#include "dock_gui.h"

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Setup new rows */

    GtkWidget *image = gtk_image_new();
    gtk_image_set_pixel_size(GTK_IMAGE(image), 50);
    gtk_list_item_set_child(listitem, image);
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    AppItem *item = gtk_list_item_get_item(listitem);
    GtkWidget *image = gtk_list_item_get_child(listitem);
    gtk_widget_set_margin_start(image, 5);
    gtk_widget_set_margin_end(image, 5);

    gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(APP_ITEM(item)->app_info));
}

static void teardown_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    AppItem *item = gtk_list_item_get_item(listitem);
    if (item == NULL)
        return;

    g_object_unref(item->app_info);
    g_object_unref(item);
}


GObject* dock_gui_init(struct Module *m)
{
    // create our custom model
    GListModel *app_model = app_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(DOCK_UI_PATH);

    GObject *w_box     = gtk_builder_get_object(builder, "dock_box");
    GObject *w_list_view     = gtk_builder_get_object(builder, "dock_lv");
    GObject *w_scroll_window     = gtk_builder_get_object(builder, "dock_sw");

    assertf(w_box != NULL, "w_box is not a widget\n");
    assertf(w_list_view != NULL, "w_list_view is not a widget\n");

    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(app_model));

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);
    g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_box;
}
