#include "apps_gui.h"
#include "gdk/gdkkeysyms.h"

static GListModel *app_model_new(void)
{
    /* fill model with some fancy data */

    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    GList *apps = g_app_info_get_all();

    GList *app = apps;
    while (app != NULL) {
        GAppInfo *app_info = app->data;

        // check if app is marked to show in menus
        if (g_app_info_should_show(app_info)) {
            AppItem *item = app_item_new(app_info);
            g_list_store_append(store, item);
        }

        app = app->next;
    }
    g_list_free(apps);
    return G_LIST_MODEL(store);
}

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Setup new rows */

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    //gtk_widget_set_margin_start(vbox, 0);
    //gtk_widget_set_margin_end(vbox, 0);

    gtk_list_item_set_child(listitem, vbox);

    GtkWidget *image = gtk_image_new();
    //gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_LARGE);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 100);

    gtk_widget_set_margin_start(image, 30);
    gtk_widget_set_margin_end(image, 30);

    gtk_box_append(GTK_BOX(vbox), image);

    GtkWidget *lb_name = gtk_label_new(NULL);
    //gtk_widget_set_margin_start(lb_name, 0);
    //gtk_widget_set_margin_end(lb_name, 0);
    //gtk_widget_set_halign(lb_name, GTK_ALIGN_CENTER);
    //gtk_widget_set_hexpand(lb_name, 0);
    //gtk_label_set_max_width_chars(GTK_LABEL(lb_name), 10);

    gtk_label_set_justify(GTK_LABEL(lb_name), GTK_JUSTIFY_CENTER);
    gtk_label_set_wrap(GTK_LABEL(lb_name), 1);
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
    //gtk_widget_set_margin_start(image, 0);
    //gtk_widget_set_margin_end(image, 0);

    //gtk_widget_set_margin_start(vbox, 0);
    //gtk_widget_set_margin_end(vbox, 0);

    //gtk_widget_set_margin_start(lb_name, 0);
    //gtk_widget_set_margin_end(lb_name, 0);

    gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(APP_ITEM(item)->app_info));
    gtk_label_set_text(GTK_LABEL(lb_name), g_app_info_get_name(APP_ITEM(item)->app_info));

    // setup drag and drop source
    GtkDragSource *drag_source = gtk_drag_source_new();
    g_signal_connect(drag_source, "prepare", G_CALLBACK(on_drag_prepare), listitem);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), listitem);
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
    //GList *args = user_data;
    //GtkSearchEntry *w_search_entry = GTK_SEARCH_ENTRY(g_list_nth(args, 1)->data);
    //GtkSelectionModel *model = GTK_SELECTION_MODEL(g_list_nth(args, 0)->data);

    //gtk_selection_model_set_selection(GTK_SELECTION_MODEL(model), 0, 0);
    //GtkSearchEntry *w_search_entry = GTK_SEARCH_ENTRY(user_data);

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

    //GtkFilter *filter = GTK_FILTER(user_data);
    gtk_filter_changed(filter, GTK_FILTER_CHANGE_DIFFERENT);
}

static void grid_view_run_app_cb(GtkGridView *self, int pos, gpointer args)
{
    GListModel *model = g_list_nth(args, 0)->data;
    GObject *win      = g_list_nth(args, 1)->data;

    //GListModel *model = G_LIST_MODEL(user_data);
    AppItem *item = g_list_model_get_item(model, pos);
    GAppInfo *app_info = item->app_info;

    // something something GError
    g_app_info_launch(app_info, NULL, NULL, NULL);
    g_signal_emit_by_name(win, "module-exit");
}

static void se_run_app_cb(GtkGridView *self, int pos, gpointer user_data)
{
    /* Run first app when enter on search entry */
    GListModel *model = G_LIST_MODEL(user_data);

    int n = g_list_model_get_n_items(G_LIST_MODEL(model));

    // if no items in selection
    if (n == 0)
        return;

    AppItem *item = g_list_model_get_item(model, 0);
    GAppInfo *app_info = item->app_info;

    // something something GError
    g_app_info_launch(app_info, NULL, NULL, NULL);
}

static int custom_sorter_cb(const void *li, const void *ri, gpointer user_data)
{
    /* Custom sorter callback that compares app names. */
    AppItem *litem = APP_ITEM((void*)li);
    AppItem *ritem = APP_ITEM((void*)ri);

    const char *lname = g_app_info_get_name(APP_ITEM(litem)->app_info);
    const char *rname = g_app_info_get_name(APP_ITEM(ritem)->app_info);

    return strcmp(lname, rname);
}

static gboolean event_key_pressed_cb(GtkEventControllerKey  *controller,
                                     guint           keyval,
                                     guint           keycode,
                                     GdkModifierType state,
                                     GtkWidget       *widget)
{
    char text[256] = "";

    if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_z ) {
        sprintf(text, "%s%c", gtk_editable_get_text(GTK_EDITABLE(widget)), keyval);
        gtk_widget_grab_focus(widget);
        gtk_editable_set_text(GTK_EDITABLE(widget), text);
        gtk_editable_set_position(GTK_EDITABLE(widget), -1);
    }
    if (keyval == GDK_KEY_BackSpace) {
        sprintf(text, "%s", gtk_editable_get_text(GTK_EDITABLE(widget)));
        if (strlen(text) > 0)
            text[strlen(text)-2] = '\0';
        gtk_widget_grab_focus(widget);
        gtk_editable_set_text(GTK_EDITABLE(widget), text);
        gtk_editable_set_position(GTK_EDITABLE(widget), -1);
            
    }

    return FALSE;
}

GObject* apps_gui_init(struct Module *m)
{
    // create our custom model
    GListModel *app_model = app_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(APPS_UI_PATH);

    GObject *w_scroll_window = gtk_builder_get_object(builder, "apps_sw");
    GObject *w_grid_view     = gtk_builder_get_object(builder, "apps_gv");
    GObject *w_se_apps       = gtk_builder_get_object(builder, "apps_se");
    GObject *w_box_apps      = gtk_builder_get_object(builder, "apps_box");


    //GtkNoSelection *no_sel;
    //args = g_list_append(args, no_sel);

    // custom filter model that filters through all fields
    GtkFilter *filter = GTK_FILTER(gtk_custom_filter_new(custom_filter_cb, w_se_apps, NULL));
    GtkFilterListModel *filter_model = gtk_filter_list_model_new(G_LIST_MODEL(app_model), filter);

    GtkSorter *sorter = GTK_SORTER(gtk_custom_sorter_new(custom_sorter_cb, NULL, NULL));
    GtkSortListModel *sort_model = gtk_sort_list_model_new(G_LIST_MODEL(filter_model), sorter);

    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(sort_model));


    GList *args = g_list_append(NULL, no_sel);
    args = g_list_append(args, m->main_win);

    // connect search input via callback to update model on change
    g_signal_connect(G_OBJECT(w_se_apps), "search-changed", G_CALLBACK(search_entry_changed_cb), filter);
    g_signal_connect(G_OBJECT(w_grid_view), "activate", G_CALLBACK(grid_view_run_app_cb), args);
    g_signal_connect(G_OBJECT(w_se_apps), "activate", G_CALLBACK(se_run_app_cb), no_sel);


    // factory creates widgets to connect model to view
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);
    g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);

    // auto focus search on keypress
    GtkEventController *key_controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(GTK_WIDGET (w_scroll_window), key_controller);
    g_signal_connect(G_OBJECT(key_controller), "key-pressed", G_CALLBACK(event_key_pressed_cb), GTK_WIDGET(w_se_apps));

    //gtk_selection_model_select_item(
    gtk_grid_view_set_model(GTK_GRID_VIEW(w_grid_view), GTK_SELECTION_MODEL(no_sel));
    gtk_grid_view_set_factory(GTK_GRID_VIEW(w_grid_view), GTK_LIST_ITEM_FACTORY(factory));

    //gtk_selection_model_select_item(GTK_SELECTION_MODEL(no_sel), 0, 0);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_grid_view));

    // set focus on text field
    gtk_widget_grab_focus(GTK_WIDGET(w_se_apps));

    return w_box_apps;
}
