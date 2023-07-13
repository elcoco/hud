#include "dock_gui.h"
#include "app_model.h"
#include "config.h"
#include "json.h"

static GListModel *app_model_new(struct Config* c)
{
    // TODO read from config file
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    GList *app;

    app = g_app_info_get_all();
    while (app != NULL) {
        if (strcmp(g_app_info_get_executable(app->data), "alacritty") == 0) {
            AppItem *item = app_item_new(app->data);
            g_list_store_append(store, item);
            break;
        }
        app = app->next;
    }

    // TODO this should be a config function mostly
    struct JSONObject *jo;
    if (config_get_child(c, "modules/dock/apps", &jo) != CONFIG_SUCCESS) {
        printf("Failed to get dock config\n");
        return G_LIST_MODEL(store);
    }

    struct JSONObject *tmp = jo->value;
    while (tmp != NULL) {
        app = g_app_info_get_all();

        while (app != NULL) {
            struct JSONObject *obj_name = json_get_path(tmp, "name");
            if (!obj_name) {
                app = app->next;
                continue;
            }
            const char *name = json_get_string(obj_name);
            const char *app_name = g_app_info_get_name(app->data);

            if (name == NULL || app_name == NULL) {
                app = app->next;
                continue;
            }

            if (strcmp(app_name, name) == 0) {
                DEBUG("from config: %s\n", name);
                AppItem *item = app_item_new(app->data);
                g_list_store_append(store, item);
                break;
            }
            app = app->next;
        }
        tmp = tmp->next;
    }

    return G_LIST_MODEL(store);
}

static void on_drag_end(GtkDragSource* self, GdkDrag* drag, gboolean delete_data, gpointer args)
{
    GtkListItem *listitem = g_list_nth(args, 0)->data;
    GListStore  *model    = g_list_nth(args, 1)->data;
    struct Config  *c    = g_list_nth(args, 2)->data;
    AppItem *appitem = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));

    assertf(listitem != NULL, "listitem == NULL");
    assertf(model != NULL, "model == NULL");
    // TODO remove path from config

    //struct JSONObject *apps;
    //if (config_get_child(c, "modules/dock/apps", &apps) < CONFIG_SUCCESS) {
    //    ERROR("Failed to get child\n");
    //    return;
    //}
    //struct JSONObject *jo = apps->value;

    struct JSONObject *rn = json_load_file(c->path);
    struct JSONObject *jo = json_get_path(rn, "modules/dock/apps");
    if (jo == NULL)
        return;

    jo = jo->value;
    struct JSONObject *child = NULL;

    while (jo != NULL) {
        child = json_get_path(jo, "name");
        if (child == NULL) {
            ERROR("Failed to get name in child\n");
            return;
        }
        DEBUG("child: %s\n", json_get_string(child));
        if (strcmp(g_app_info_get_name(appitem->app_info), json_get_string(child)) == 0)
            break;
        jo = jo->next;
    }

    if (jo) {
        printf("REMOVED!\n");
        json_object_destroy(jo);
        json_object_to_file(rn, c->path, 4);
    }
    else
        DEBUG("Failed to Removed item\n");


    //buf path[256] = "";
    //sprintf(path,, "modules/dock/apps/%s"
    //config_remove_child(c, 
        





    g_list_store_remove(model, gtk_list_item_get_position(GTK_LIST_ITEM(listitem)));
}

static gboolean on_drop_icon(GtkDropTarget *target, const GValue *value, double x, double y, gpointer args)
{
    GtkListItem *li_target = g_list_nth(args, 0)->data;
    GListModel *model = g_list_nth(args, 1)->data;
    struct Config  *c    = g_list_nth(args, 2)->data;

    GObject *listitem_target = g_value_get_object(value);
    AppItem *appitem = gtk_list_item_get_item(GTK_LIST_ITEM(listitem_target));

    int pos = gtk_list_item_get_position(GTK_LIST_ITEM(li_target));
    // remove empty unit
    DEBUG("DROP :: Remove empty unit @ %d\n", pos);
    g_list_store_remove(G_LIST_STORE(model), pos);

    DEBUG("DROP :: Insert unit @ %d\n", pos);
    g_list_store_insert(G_LIST_STORE(model), pos, G_OBJECT(appitem));

    struct JSONObject *app = json_object_init_object(NULL, NULL);
    json_object_init_string(app, "type", "appinfo");
    json_object_init_string(app, "name", g_app_info_get_name(appitem->app_info));
    if (config_array_insert(c, "modules/dock/apps", app, pos) < CONFIG_SUCCESS)
        ERROR("Failed to write app to config\n");

    json_print(app, 0);
    DEBUG("Insert $ %d\n", pos);
    // TODO this now appends but should insert
    //config_set_str(c, "modules/dock/apps/[?]", "type", "appinfo");
    //config_set_str(c, "modules/dock/apps/[-1]", "name", g_app_info_get_name(appitem->app_info));

    return 1;

}

void remove_empty(GListModel *model, int pos_exclude)
{
    // remove all empty items except for item at pos_exclude
    //for (int i=0 ; i<g_list_model_get_n_items(model) ; i++) {
    for (int i=g_list_model_get_n_items(model)-1 ; i>=0 ; i--) {
        if (i == pos_exclude)
            continue;

        AppItem *item = APP_ITEM(g_list_model_get_object(model, i));
        if (item->app_info == NULL) {
            printf("removing item: %d\n", i);
            g_list_store_remove(G_LIST_STORE(model), i);
        }
    }
}

static void on_motion_icon(GtkDropControllerMotion *controller, double x, double y, gpointer args)
{
    GtkListItem *li_target = g_list_nth(args, 0)->data;
    GListModel *model = g_list_nth(args, 1)->data;

    GtkWidget *target_widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    AppItem *appitem = gtk_list_item_get_item(GTK_LIST_ITEM(li_target));

    int width = gtk_widget_get_width(GTK_WIDGET(target_widget));
    int pos = gtk_list_item_get_position(GTK_LIST_ITEM(li_target));

    // we are hovering an empty drop space
    if (appitem->app_info == NULL)
        return;

    // get items on the left and right of current item
    AppItem *appitem_left = APP_ITEM(g_list_model_get_object(model, pos-1)); 
    AppItem *appitem_right = APP_ITEM(g_list_model_get_object(model, pos+1)); 

    // the empty icon that indicates a drop space
    AppItem *item = app_item_new(NULL);

    // check if we're on the left or right side of icon and insert empty space before or after
    if (x <= (int)(width/2)) {
        if (appitem_left == NULL || appitem_left->app_info != NULL) {
            //DEBUG("ENTER :: Insert left @ %d\n", pos);
            g_list_store_insert(G_LIST_STORE(model), pos, item);
            remove_empty(model, pos);
        }
    }
    else {
        if (appitem_right == NULL || appitem_right->app_info != NULL) {
            //DEBUG("ENTER :: Insert right @ %d\n", pos +1);
            g_list_store_insert(G_LIST_STORE(model), pos+1, item);
            remove_empty(model, pos+1);
        }
    }
}

/*
static gboolean on_drop_dock(GtkDropTarget *target, const GValue *value, double x, double y, gpointer args)
{
    GListStore  *model = g_list_nth(args, 0)->data;
    struct Config *c   = g_list_nth(args, 1)->data;

    GtkListItem *listitem = GTK_LIST_ITEM(g_value_get_object(value));
    AppItem *item = gtk_list_item_get_item(listitem);


    // todo write to config file

    config_set_str(c, "modules/dock/apps/[?]", "type", "appinfo");
    config_set_str(c, "modules/dock/apps/[-1]", "name", g_app_info_get_name(item->app_info));
    g_list_store_append(G_LIST_STORE(model), item);

    //config_set_number(c, "modules/dock/apps/[-1]", "position", gtk_list_item_get_position(GTK_LIST_ITEM(listitem)));
    //int pos_target = gtk_list_item_get_position(GTK_LIST_ITEM(listitem_target));

    printf("DOCK DRAG_END :: %s\n", g_app_info_get_name(item->app_info));
    return TRUE;
}
*/

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

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer args)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    GListStore  *model = g_list_nth(args, 0)->data;
    struct Config *c   = g_list_nth(args, 1)->data;

    // custom model item that contains the data
    AppItem *item = gtk_list_item_get_item(listitem);
    GtkWidget *box = gtk_list_item_get_child(listitem);
    GtkWidget *image = gtk_widget_get_first_child(box);

    if (item->app_info)
        gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(APP_ITEM(item)->app_info));
    else
        gtk_image_set_from_icon_name(GTK_IMAGE(image), "list-add");


    // setup drag
    GtkDragSource *drag_source = gtk_drag_source_new();
    g_signal_connect(drag_source, "prepare", G_CALLBACK(on_drag_prepare), listitem);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), listitem);


    GList *args_drag = g_list_append(NULL, listitem);
    args_drag = g_list_append(args_drag, model);
    args_drag = g_list_append(args_drag, c);

    g_signal_connect(drag_source, "drag-end", G_CALLBACK(on_drag_end), args_drag);
    gtk_widget_add_controller(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(drag_source));

    // controller handles events when mouse is over widgets during drag and drop
    GtkEventController *controller = gtk_drop_controller_motion_new();
    gtk_widget_add_controller(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(controller));
    g_signal_connect(controller, "motion", G_CALLBACK(on_motion_icon), args_drag);
    //g_signal_connect(controller, "leave", G_CALLBACK(on_leave_icon), args_drag);


    // icon can be dropped on other icon to reorder the dock
    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_OBJECT, GDK_ACTION_COPY);

    g_signal_connect(target, "drop", G_CALLBACK(on_drop_icon), args_drag);
    gtk_widget_add_controller(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(target));
}

static void teardown_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    AppItem *item = gtk_list_item_get_item(listitem);
    if (item == NULL)
        return;

    g_object_unref(item->app_info);
    g_object_unref(item);
}

static void list_view_run_app_cb(GtkGridView *self, int pos, gpointer args)
{
    GListModel *model = g_list_nth(args, 0)->data;
    GObject *win      = g_list_nth(args, 1)->data;

    AppItem *item = g_list_model_get_item(model, pos);
    GAppInfo *app_info = item->app_info;

    // something something GError
    g_app_info_launch(app_info, NULL, NULL, NULL);

    g_signal_emit_by_name(win, "module-exit");
}

GObject* dock_gui_init(struct Module *m)
{
    // create our custom model
    GListModel *app_model = app_model_new(m->config);

    GtkBuilder *builder = gtk_builder_new_from_resource(DOCK_UI_PATH);

    GObject *w_box     = gtk_builder_get_object(builder, "dock_box");
    GObject *w_list_view     = gtk_builder_get_object(builder, "dock_lv");
    //GObject *w_scroll_window     = gtk_builder_get_object(builder, "dock_sw");

    assertf(w_box != NULL, "w_box is not a widget\n");
    assertf(w_list_view != NULL, "w_list_view is not a widget\n");

    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(app_model));

    GList *bind_args = g_list_append(NULL, app_model);
    bind_args = g_list_append(bind_args, m->config);

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), bind_args);
    g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));


    GList *activate_args = g_list_append(NULL, no_sel);
    activate_args = g_list_append(activate_args, m->main_win);
    g_signal_connect(G_OBJECT(w_list_view), "activate", G_CALLBACK(list_view_run_app_cb), activate_args);


    //GtkDropTarget *target = gtk_drop_target_new(G_TYPE_OBJECT, GDK_ACTION_COPY);
    //GList *args = g_list_append(NULL, app_model);
    //args = g_list_append(args, m->config);
    //g_signal_connect(target, "drop", G_CALLBACK(on_drop_dock), args);
    //gtk_widget_add_controller(GTK_WIDGET(w_list_view), GTK_EVENT_CONTROLLER(target));


    //gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_box;
}
