#include "mpris_gui.h"

struct _MPRISItemClass {
    GObjectClass parent_class;
};

G_DEFINE_TYPE(MPRISItem, mpris_item, G_TYPE_OBJECT)
static void mpris_item_init(MPRISItem *item)
{
}

static void mpris_item_class_init(MPRISItemClass *class)
{
}

static MPRISItem *mpris_item_new(struct MprisPlayer *mp)
{
    MPRISItem *item;
    item = g_object_new(MPRIS_TYPE_ITEM, NULL);
    item->mp = mp;

    return item;
}

static GListModel *mpris_model_new(void)
{
    /* Create model now, fill later */
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    return G_LIST_MODEL(store);
}

static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    GtkBuilder *builder = gtk_builder_new_from_resource(MPRIS_UI_PATH);
    GObject *w_box = gtk_builder_get_object(builder, "mpris_player_box");
    gtk_list_item_set_child(listitem, GTK_WIDGET(w_box));
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    MPRISItem *item = gtk_list_item_get_item(listitem);

    GtkWidget *main_box = gtk_list_item_get_child(listitem);
    GtkWidget *header_box = gtk_widget_get_first_child(main_box);
    GtkWidget *artwork_img = gtk_widget_get_first_child(header_box);

    GtkWidget *label_box = gtk_widget_get_next_sibling(artwork_img);
    GtkWidget *artist_lb = gtk_widget_get_first_child(label_box);
    GtkWidget *track_lb = gtk_widget_get_next_sibling(artist_lb);

    GtkWidget *controls_box = gtk_widget_get_next_sibling(header_box);
    GtkWidget *controls = gtk_widget_get_first_child(controls_box);

    gtk_range_set_range(GTK_RANGE(controls), 0, item->mp->metadata.length);
    gtk_range_set_value(GTK_RANGE(controls), item->mp->metadata.length/2);
    gtk_range_set_fill_level(GTK_RANGE(controls), item->mp->metadata.length/2);

    gtk_label_set_markup(GTK_LABEL(artist_lb), item->mp->metadata.artist);
    gtk_label_set_markup(GTK_LABEL(track_lb), item->mp->metadata.title);
    gtk_image_set_from_file(GTK_IMAGE(artwork_img), item->mp->metadata.art_url);
}


int mpris_get_data_thread(void *args)
{
    GListModel *model = G_LIST_MODEL(g_list_nth(args, 0)->data);
    struct Module *m = g_list_nth(args, 1)->data;

    if (module_is_locked(m))
        return 1;

    printf("getting the dataz\n");

    g_list_store_remove_all(G_LIST_STORE(model));

    struct MprisPlayer *mp = mpris_players_load();
    if (mp == NULL) {
        printf("Failed to get players\n");
        return -1;
    }

    while (mp != NULL) {

        MPRISItem *item = mpris_item_new(mp);
        g_list_store_append(G_LIST_STORE(model), item);

        mp = mp->next;
    }

}

GObject* mpris_gui_init(struct Module *m)
{
    GListModel *model = mpris_model_new();
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    GtkNoSelection *no_sel = gtk_no_selection_new(G_LIST_MODEL(model));
    GtkWidget *lv = gtk_list_view_new(GTK_SELECTION_MODEL(no_sel), factory);

    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind",  G_CALLBACK(bind_cb), NULL);
    //g_signal_connect(factory, "teardown",  G_CALLBACK(teardown_cb), NULL);


    GList *args = g_list_append(NULL, model);
    args = g_list_append(args, m);

    mpris_get_data_thread(args);
    g_timeout_add(MPRIS_UPDATE_INTERVAL_MS, mpris_get_data_thread, args);


    return G_OBJECT(lv);

}
