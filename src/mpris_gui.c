#include "mpris_gui.h"
#include "mpris.h"

// TODO create MprisPlayer structs and tye them to the MPRISItem
// instead of creating new struct everytime just update them
// this way we can make the struct attributes into object properties.
// Now we use properties in the MPRISItem which is annoying since we have
// to do double work
// Mpris structs need to be added to the MPRISItem object before any
// of its methods are called because that would pass uninitialized values
// around

// use this to block signals when moving slider to not get into eternal loop
// Should be done with the g_signal_handlers_block_by_func funtion but that doesn't
// work for some reason
_Atomic int block_signals = 0;

// store the headnode here, will be updated in mpris_get_data_thread()
struct MPRISPlayer *head = NULL;
struct MPRISPlayer **mp = &head;

static void on_scale_value_changed(GObject *self, gpointer user_data);

// https://docs.gtk.org/gobject/concepts.html#object-properties
enum MprisItemProperty {
    PROP_ARTIST = 1,
    PROP_TITLE,
    PROP_ART_URL,
    PROP_POSITION,
    PROP_LENGTH,
    PROP_STATUS,
    PROP_TRACK_ID,
    N_PROPS
};

static GParamSpec *obj_properties[N_PROPS] = { NULL, };

static void mpris_item_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  MPRISItem *self = MPRIS_ITEM (object);

  switch ((enum MprisItemProperty) property_id)
    {
    case PROP_ARTIST:
        self->mp->metadata->artist = g_value_dup_string(value);
        break;

    case PROP_TITLE:
        printf("set prop TITLE => %s\n", g_value_dup_string(value));
        self->mp->metadata->title = g_value_dup_string(value);
        break;

    case PROP_ART_URL:
        self->mp->metadata->art_url = g_value_dup_string(value);
        break;

    case PROP_POSITION:
        self->mp->properties->position = g_value_get_uint64(value);
        break;

    case PROP_LENGTH:
        self->mp->metadata->length = g_value_get_uint64(value);
        break;

    case PROP_STATUS:
        self->mp->properties->status = g_value_get_int(value);
        break;

    case PROP_TRACK_ID:
        self->mp->metadata->track_id = g_value_dup_string(value);
        break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void mpris_item_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  MPRISItem *self = MPRIS_ITEM (object);

  switch ((enum MprisItemProperty) property_id) {
    case PROP_ARTIST:
        g_value_set_string(value, self->mp->metadata->artist);
        break;

    case PROP_TITLE:
        g_value_set_string(value, self->mp->metadata->title);
        break;

    case PROP_ART_URL:
        g_value_set_string(value, self->mp->metadata->art_url);
        break;

    case PROP_POSITION:
        g_value_set_uint64(value, self->mp->properties->position);
        break;

    case PROP_LENGTH:
        g_value_set_uint64(value, self->mp->metadata->length);
        break;

    case PROP_STATUS:
        g_value_set_int(value, self->mp->properties->status);
        break;

    case PROP_TRACK_ID:
        g_value_set_string(value, self->mp->metadata->track_id);
        break;

    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

struct _MPRISItemClass {
    GObjectClass parent_class;
};

G_DEFINE_TYPE(MPRISItem, mpris_item, G_TYPE_OBJECT)
static void mpris_item_init(MPRISItem *item)
{
}

static void mpris_item_class_init(MPRISItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = mpris_item_set_property;
    object_class->get_property = mpris_item_get_property;

    obj_properties[PROP_ARTIST] = g_param_spec_string("artist",
                                                      "Artist",
                                                      "Name of the file to load and display from.",
                                                      "default_artist_name",
                                                      G_PARAM_READWRITE);
                                                      //G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
                                                      //
    obj_properties[PROP_TITLE] = g_param_spec_string("title",
                                                     "Title",
                                                     "Name of the file to load and display from.",
                                                     "default_title",
                                                     G_PARAM_READWRITE);

    obj_properties[PROP_ART_URL] = g_param_spec_string("art_url",
                                                      "Art url",
                                                      "Name of the file to load and display from.",
                                                      "default_art_url",
                                                      G_PARAM_READWRITE);

    obj_properties[PROP_POSITION] = g_param_spec_uint64("position",
                                                      "Position",
                                                      "Name of the file to load and display from.",
                                                      0,
                                                      10000000000,
                                                      0,
                                                      G_PARAM_READWRITE);

    obj_properties[PROP_LENGTH] = g_param_spec_uint64("length",
                                                      "Length",
                                                      "Name of the file to load and display from.",
                                                      0,
                                                      10000000000,
                                                      0,
                                                      G_PARAM_READWRITE);
    obj_properties[PROP_STATUS] = g_param_spec_int("status",
                                                      "Status",
                                                      "Name of the file to load and display from.",
                                                      0,
                                                      10,
                                                      0,
                                                      G_PARAM_READWRITE);

    obj_properties[PROP_TRACK_ID] = g_param_spec_string("track_id",
                                                      "Track ID",
                                                      "Name of the file to load and display from.",
                                                      "default_art_url",
                                                      G_PARAM_READWRITE);
    g_object_class_install_properties(object_class,
                                      N_PROPS,
                                      obj_properties);
}

static MPRISItem *mpris_item_new(struct MPRISPlayer *mp)
{
    MPRISItem *item;
    //GValue gartist = G_VALUE_INIT;
    //g_value_init(&gartist, G_TYPE_STRING);
    //g_value_set_string (&gartist, mp->metadata.artist);
    //printf ("property: %s\n", g_value_get_string (&gartist));
    //const char *names[] = {"artist"};
    //const GValue* values = {&gartist};
    printf("creating item with namespace: %s\n", mp->namespace);

    //item = MPRIS_ITEM(g_object_new_with_properties(MPRIS_TYPE_ITEM, 1, names, values));
    item = g_object_new(MPRIS_TYPE_ITEM, NULL);
    item->mp = mp;
    item->namespace = strdup(mp->namespace);
    item->first_run = 1;
    printf("endof init\n");

    //update_prop_str(G_OBJECT(item), "artist", mp->metadata.artist);
    //update_prop_str(G_OBJECT(item), "title", mp->metadata->title, 1);
    //update_prop_str(G_OBJECT(item), "art_url", mp->metadata.art_url);
    //update_prop_uint64(G_OBJECT(item), "position", mp->properties.position);
    //update_prop_uint64(G_OBJECT(item), "length", mp->metadata.length);
    //update_prop_int(G_OBJECT(item), "status", mp->properties.status);
    //item->artist = strdup(mp->metadata.artist);

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

static void on_position_changed(GObject *self, void *param, gpointer user_data)
{
    /* Set scale range when position changes */
    GValue glength = G_VALUE_INIT;
    g_value_init(&glength, G_TYPE_UINT64);
    g_object_get_property(self, "length", &glength);
    gtk_range_set_range(GTK_RANGE(user_data), 0, g_value_get_uint64(&glength));

    GValue gposition = G_VALUE_INIT;
    g_value_init(&gposition, G_TYPE_UINT64);
    g_object_get_property(self, "position", &gposition);

    // block value-changed signal here to not end up in a loop
    block_signals = 1;
    gtk_range_set_value(GTK_RANGE(user_data), g_value_get_uint64(&gposition));
    block_signals = 0;
}

static void on_status_changed(GObject *self, void *param, gpointer user_data)
{
    MPRISItem *item = MPRIS_ITEM(self);
    if (item->mp->properties->status == MPRIS_STATUS_PAUSED)
        gtk_button_set_icon_name(GTK_BUTTON(user_data), "media-playback-start-symbolic");
    else if (item->mp->properties->status == MPRIS_STATUS_PLAYING)
        gtk_button_set_icon_name(GTK_BUTTON(user_data), "media-playback-pause-symbolic");
}

static void on_toggle_clicked(GObject *self, void *param, gpointer user_data)
{
    MPRISItem *item = MPRIS_ITEM(user_data);
    if (item->mp->properties->status == MPRIS_STATUS_PAUSED)
        mpris_player_play(item->namespace);
    else if (item->mp->properties->status == MPRIS_STATUS_PLAYING)
        mpris_player_pause(item->namespace);
}

static void on_next_clicked(GObject *self, void *param, gpointer user_data)
{
    mpris_player_next(MPRIS_ITEM(user_data)->namespace);
}

static void on_prev_clicked(GObject *self, void *param, gpointer user_data)
{
    mpris_player_prev(MPRIS_ITEM(user_data)->namespace);
}

static void on_scale_value_changed(GObject *self, gpointer user_data)
{
    // look at block so we don't end up in slider loop of dooooom!
    if (block_signals)
        return;

    uint64_t val = gtk_range_get_value(GTK_RANGE(self));
    MPRISItem *item = MPRIS_ITEM(user_data);
    mpris_player_set_position(item->namespace, item->mp->metadata->track_id, val);
}

static void bind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data)
{
    MPRISItem *item = gtk_list_item_get_item(listitem);
    printf("binding item with namespace: %s\n", item->namespace);

    GtkWidget *main_box = gtk_list_item_get_child(listitem);
    GtkWidget *header_box = gtk_widget_get_first_child(main_box);
    GtkWidget *artwork_img = gtk_widget_get_first_child(header_box);

    GtkWidget *label_box = gtk_widget_get_next_sibling(artwork_img);
    GtkWidget *artist_lb = gtk_widget_get_first_child(label_box);
    GtkWidget *title_lb = gtk_widget_get_next_sibling(artist_lb);

    GtkWidget *btn_box = gtk_widget_get_next_sibling(header_box);
    GtkWidget *prev_btn = gtk_widget_get_first_child(btn_box);
    GtkWidget *toggle_btn = gtk_widget_get_next_sibling(prev_btn);
    GtkWidget *next_btn = gtk_widget_get_next_sibling(toggle_btn);

    GtkWidget *controls_box = gtk_widget_get_next_sibling(btn_box);
    GtkWidget *controls = gtk_widget_get_first_child(controls_box);

    // callback to adjust range and update position
    g_signal_connect(item, "notify::position",  G_CALLBACK(on_position_changed), controls);
    g_signal_connect(item, "notify::status",  G_CALLBACK(on_status_changed), toggle_btn);
    g_signal_connect(toggle_btn, "clicked",  G_CALLBACK(on_toggle_clicked), item);
    g_signal_connect(next_btn, "clicked",  G_CALLBACK(on_next_clicked), item);
    g_signal_connect(prev_btn, "clicked",  G_CALLBACK(on_prev_clicked), item);
    g_signal_connect(controls, "value-changed",  G_CALLBACK(on_scale_value_changed), item);

    // Bind the artist property from our custom object to the label property that displays text
    // When we update the property the label will automatically change
    g_object_bind_property(item, "artist", artist_lb, "label", 0);
    g_object_bind_property(item, "title", title_lb, "label", 0);
    g_object_bind_property(item, "art_url", artwork_img, "file", 0);
}

MPRISItem* mpris_model_contains_namespace(GListModel *model, char *namespace)
{
    /* check if model contains an item with the same dbus namespace */
    for (int i=0 ; i<g_list_model_get_n_items(model) ; i++) {
        MPRISItem *item = g_list_model_get_item(model, i);
        if (strcmp(item->namespace, namespace) == 0)
            return item;
    }
    return NULL;
}

int mpris_model_remove_players(GListModel *model, struct MPRISPlayer *mp_head)
{
    /* Check model if it contains items that do not exist in our data and remove them */

    for (int i=0 ; i<g_list_model_get_n_items(model) ; i++) {
        MPRISItem *item = g_list_model_get_item(model, i);

        struct MPRISPlayer *mp = mp_head;

        int match = 0;
        while (mp != NULL) {
            if (strcmp(item->namespace, mp->namespace) == 0) {
                match = 1;
                break;
            }
            mp = mp->next;
        }
        if (!match)
            g_list_store_remove(G_LIST_STORE(model), i);
    }
    return 0;
}

int mpris_get_data_thread(void *args)
{
    /*
    MODEL:
    GListStore->GListModel->GObject
        contains items: MPRISItem->GObject

    VIEW:
    GtkListView->GtkListBase->GtkWidget->GInitialyUnowned->GObject
        contains items: GtkListItem->GObject

    GtkListItem and MPRISItem are linked by factory using the setup() and bind() functions

    GtkListItem has a reference to MPRISItem:
        gtk_list_item_get_item(GtkListItem *listitem)

    */
    GListModel *model = G_LIST_MODEL(g_list_nth(args, 0)->data);
    struct Module *m = g_list_nth(args, 1)->data;

    if (module_is_locked(m))
        return 1;

    struct MPRISPlayer *mp_head = mpris_player_load_all(*mp);

    // check if a player is removed
    mpris_model_remove_players(model, mp_head);

    if (mp_head == NULL) {
        printf("No players found\n");
        return -1;
    }

    // check if a new player is found or that the player should be updated with new data
    struct MPRISPlayer *mp = mp_head;
    while (mp != NULL) {
        MPRISItem *item;
        MPRISItem *found = mpris_model_contains_namespace(model, mp->namespace);

        if (found != NULL) {
            item = found;
        }
        else {
            item = mpris_item_new(mp);
            g_list_store_append(G_LIST_STORE(model), item);
        }

        update_prop_str(G_OBJECT(item), "artist", mp->metadata->artist, item->first_run);
        update_prop_str(G_OBJECT(item), "title", mp->metadata->title, item->first_run);
        update_prop_str(G_OBJECT(item), "art_url", mp->metadata->art_url, item->first_run);
        update_prop_uint64(G_OBJECT(item), "length", mp->metadata->length, item->first_run);
        update_prop_uint64(G_OBJECT(item), "position", mp->properties->position, item->first_run);
        update_prop_int(G_OBJECT(item), "status", mp->properties->status, item->first_run);
        update_prop_str(G_OBJECT(item), "track_id", mp->metadata->track_id, item->first_run);
        mp = mp->next;

        item->first_run = 0;
    }
    return 1;
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

    //mpris_get_data_thread(args);
    g_timeout_add(MPRIS_UPDATE_INTERVAL_MS, mpris_get_data_thread, args);

    return G_OBJECT(lv);
}
