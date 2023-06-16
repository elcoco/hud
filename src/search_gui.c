#include "search_gui.h"
#include "rg.h"

//https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// to bind do window use: gtk_window_set_display (GTK_WINDOW (window),
//
//


struct _SearchItemClass {
    GObjectClass parent_class;
};

G_DEFINE_TYPE(SearchItem, search_item, G_TYPE_OBJECT)
static void search_item_init(SearchItem *item)
{
}

static void search_item_class_init(SearchItemClass *class)
{
}

static SearchItem *search_item_new(struct RGLine *l)
{
    SearchItem *item;

    item = g_object_new(SEARCH_TYPE_ITEM, NULL);
    item->text = l->text;
    item->path = l->path;
    item->lineno = l->lineno;

    return item;
}

static GListModel *search_model_new(void)
{
    /* fill model with some fancy data */
    //const char *text = "bever";

    // get data
    //struct RGLine *l = rgline_init(NULL);
    
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    return G_LIST_MODEL(store);
}


static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkBuilder *builder = gtk_builder_new_from_resource(SEARCH_ITEM_UI_PATH);

    GObject *box          = gtk_builder_get_object(builder, "search_item_box");
    //GObject *path_lb          = gtk_builder_get_object(builder, "search_item_path_lb");
    //GObject *text_lb          = gtk_builder_get_object(builder, "search_item_text_lb");

    gtk_list_item_set_child(listitem, GTK_WIDGET(box));
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    /* Bind model items to view items.
     * In other words fill in the widget content with data from the model
     */

    // custom model item that contains the data
    SearchItem *item = gtk_list_item_get_item(listitem);

    GtkWidget *box = gtk_list_item_get_child(listitem);
    GtkWidget *box_header = gtk_widget_get_first_child(GTK_WIDGET(box));
    GtkWidget *path_lb = gtk_widget_get_first_child(GTK_WIDGET(box_header));
    GtkWidget *lineno_lb = gtk_widget_get_next_sibling(GTK_WIDGET(path_lb));

    GtkWidget *text_lb = gtk_widget_get_next_sibling(GTK_WIDGET(box_header));
    
    gtk_label_set_text(GTK_LABEL(path_lb), SEARCH_ITEM(item)->path);
    gtk_label_set_text(GTK_LABEL(text_lb), SEARCH_ITEM(item)->text);

    char buf[128] = "";
    sprintf(buf, "@ %d", SEARCH_ITEM(item)->lineno);
    gtk_label_set_text(GTK_LABEL(lineno_lb), buf);
}



static void search_entry_changed_cb(void* self, gpointer user_data)
{
    /* Is called when search entry is changed and notifies users of filter to update the models */
    printf("search changed\n");
    GListModel *model = G_LIST_MODEL(user_data);
    g_list_store_remove_all(G_LIST_STORE(model));

    const char *inp_txt = gtk_editable_get_text(GTK_EDITABLE(self));

    struct RGLine *l = rgline_init(NULL);
    if (rg_request(inp_txt, l) < 0) {
        fprintf(stderr, "Error while calling ripgrep\n");
        return;
    }


    while (l != NULL) {
        SearchItem *item = search_item_new(l);
        g_list_store_append(G_LIST_STORE(model), item);
        l = l->next;
    }
}

GObject* search_gui_init()
{
    // create our custom model
    GListModel *search_model = search_model_new();

    GtkBuilder *builder = gtk_builder_new_from_resource(SEARCH_UI_PATH);

    GObject *w_vbox          = gtk_builder_get_object(builder, "search_box");
    GObject *w_scroll_window = gtk_builder_get_object(builder, "search_sw");
    GObject *w_list_view     = gtk_builder_get_object(builder, "search_lv");
    GObject *w_search_entry  = gtk_builder_get_object(builder, "search_se");

    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(search_model));

    // connect search input via callback to update model on change
    g_signal_connect(G_OBJECT(w_search_entry), "search-changed", G_CALLBACK(search_entry_changed_cb), search_model);

    // factory creates widgets to connect model to view
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);

    gtk_list_view_set_model(GTK_LIST_VIEW(w_list_view), GTK_SELECTION_MODEL(no_sel));
    gtk_list_view_set_factory(GTK_LIST_VIEW(w_list_view), GTK_LIST_ITEM_FACTORY(factory));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll_window), GTK_WIDGET(w_list_view));

    return w_vbox;
}

