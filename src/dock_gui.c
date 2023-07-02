#include "dock_gui.h"
#include "app_model.h"

static GListModel *app_model_new(void)
{
    // TODO read from config file
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);

    GList *app = g_app_info_get_all();
    while (app != NULL) {
        if (strcmp(g_app_info_get_executable(app->data), "xfce4-about") == 0) {
            AppItem *item = app_item_new(app->data);
            g_list_store_append(store, item);
            break;
        }
        app = app->next;
    }

    return G_LIST_MODEL(store);
}

static void on_drag_end(GtkDragSource* self, GdkDrag* drag, gboolean delete_data, gpointer args)
{
    GtkListItem *listitem = g_list_nth(args, 0)->data;
    GListStore  *model    = g_list_nth(args, 1)->data;

    assertf(listitem != NULL, "listitem == NULL");
    assertf(model != NULL, "model == NULL");

    g_list_store_remove(model, gtk_list_item_get_position(GTK_LIST_ITEM(listitem)));
}

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Setup new rows */

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *image = gtk_image_new();
    gtk_box_append(GTK_BOX(box), image);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 50);
    gtk_list_item_set_child(listitem, box);

    gtk_widget_set_margin_start(image, 5);
    gtk_widget_set_margin_end(image, 5);
    gtk_widget_set_margin_top(image, 10);
    gtk_widget_set_margin_bottom(image, 10);
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer model)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    AppItem *item = gtk_list_item_get_item(listitem);
    GtkWidget *box = gtk_list_item_get_child(listitem);
    GtkWidget *image = gtk_widget_get_first_child(box);

    gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(APP_ITEM(item)->app_info));

    // setup drag
    GtkDragSource *drag_source = gtk_drag_source_new();
    g_signal_connect(drag_source, "prepare", G_CALLBACK(on_drag_prepare), listitem);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), listitem);


    GList *args = g_list_append(NULL, listitem);
    args = g_list_append(args, model);

    g_signal_connect(drag_source, "drag-end", G_CALLBACK(on_drag_end), args);
    gtk_widget_add_controller(GTK_WIDGET(image), GTK_EVENT_CONTROLLER(drag_source));
}

static void teardown_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    AppItem *item = gtk_list_item_get_item(listitem);
    if (item == NULL)
        return;

    g_object_unref(item->app_info);
    g_object_unref(item);
}


static gboolean on_drop (GtkDropTarget *target, const GValue *value, double x, double y, gpointer user_data)
{
    GtkListItem *listitem = GTK_LIST_ITEM(g_value_get_object(value));
    AppItem *item = gtk_list_item_get_item(listitem);
    printf("dragging end: %s\n", g_app_info_get_name(item->app_info));

    // todo write to config file

    g_list_store_append(G_LIST_STORE(user_data), item);
    return TRUE;
}

GObject* dock_gui_init(struct Module *m)
{
    // create our custom model
    GListModel *app_model = app_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(DOCK_UI_PATH);

    GObject *w_box     = gtk_builder_get_object(builder, "dock_box");
    GObject *w_list_view     = gtk_builder_get_object(builder, "dock_lv");
    //GObject *w_scroll_window     = gtk_builder_get_object(builder, "dock_sw");

    assertf(w_box != NULL, "w_box is not a widget\n");
    assertf(w_list_view != NULL, "w_list_view is not a widget\n");

    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(app_model));

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), app_model);
    g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_OBJECT, GDK_ACTION_COPY);
    g_signal_connect(target, "drop", G_CALLBACK(on_drop), app_model);
    gtk_widget_add_controller(GTK_WIDGET(w_list_view), GTK_EVENT_CONTROLLER(target));


    //gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_box;
}
