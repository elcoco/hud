#include "mpris.h"
#include "mpris_gdbus.h"

static struct MPRISPlayer* mpris_player_init(struct MPRISPlayer *prev, char *namespace);
static int mpris_player_load(struct MPRISPlayer *mp);
static int get_player_namespaces(char **buf, int max_players);
static void mpris_player_set_string_from_gvariant(char **buf, GVariant *value, const char *default_value);
static int mpris_player_set_string(char **buf, const char *value, const char *default_value);
static struct MPRISPlayer* mpris_player_get_head(struct MPRISPlayer *mp);
static struct MPRISPlayer* mpris_player_find_in_ll(struct MPRISPlayer *mp_head, const char *namespace);


static struct MPRISPlayer* mpris_player_find_in_ll(struct MPRISPlayer *mp_head, const char *namespace)
{
    struct MPRISPlayer *mp = mp_head;
    while (mp != NULL) {
        if (strncmp(mp->namespace, namespace, strlen(namespace)) == 0) {
            return mp;
        }
        mp = mp->next;
    }
    return NULL;
}

static struct MPRISPlayer *mpris_player_remove_old(struct MPRISPlayer *mp_head, char **namespaces, size_t len)
{
    /* Remove items from linked list that are not found in new data. */
    struct MPRISPlayer *mp = mp_head;
    while (mp != NULL) {
        int found = 0;
        for (int i=0 ; i<len ; i++) {
            if (namespaces[i] == NULL)
                break;
            if (strncmp(mp->namespace, namespaces[i], strlen(namespaces[i])) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            //printf("REMOVING: %s\n", mp->namespace);
            mp = mpris_player_destroy(mp);
            mp_head = mp;
            continue;
        }

        mp = mp->next;
    }
    return mp_head;
}


struct MPRISPlayer* mpris_player_load_all(struct MPRISPlayer *mp_head)
{
    /* Load all structs.
     * if mp_head != NULL, search in linked list for existing structs and update them.
     * If mp_head != NULL, the linked list will be matched against found dbus data and
     *   items will be removed from linked list if necessary */

    char *namespaces[MAX_PLAYERS+1] = {NULL};
    get_player_namespaces(namespaces, MAX_PLAYERS);

    // if a previous linked list was passed in we want to remove the items that are absent in the new data
    // before updating everything
    if (mp_head != NULL)
        mp_head = mpris_player_remove_old(mp_head, namespaces, MAX_PLAYERS);

    // find linked list tail so we can append new items if found
    struct MPRISPlayer *mp_tail = mp_head;
    while (mp_tail != NULL && mp_tail->next != NULL)
        mp_tail = mp_tail->next;

    for (int i=0 ; i<MAX_PLAYERS ; i++) {
        if (namespaces[i] == NULL)
            break;

        // search linked list and update if there is a match
        struct MPRISPlayer *mp_found = mpris_player_find_in_ll(mp_head, namespaces[i]);
        if (mp_found != NULL) {
            mpris_player_load(mp_found);
        }
        else {
            mp_tail = mpris_player_init(mp_tail, namespaces[i]);
            mpris_player_load(mp_tail);

            if (mp_head == NULL)
                mp_head = mp_tail;
        }
        free(namespaces[i]);
    }

    return mp_head;
}

struct MPRISPlayer* mpris_player_destroy(struct MPRISPlayer *mp)
{
    /* Destroy node in linked list and return head node
     * If list is empty, return NULL */

    // handle case where this is the current head, update all nodes with new head
    if (mp->prev == NULL && mp->next != NULL) {
        struct MPRISPlayer *head = mp->next;
        struct MPRISPlayer *tmp = head;

        // update head in linked list
        while (tmp != NULL) {
            tmp->head = head;
            tmp = tmp->next;
        }
    }

    struct MPRISPlayer *ret;

    // node is head
    if (mp->prev == NULL && mp->next != NULL)
        ret = mp->next;

    // node is only node
    else if (mp->prev == NULL && mp->next == NULL)
        ret = NULL;

    else
        ret = mpris_player_get_head(mp);

    // disconnect node from linked list so we can safely free it
    if (mp->prev) {
        if (mp->next)
            mp->prev->next = mp->next;
        else
            mp->prev->next = NULL;
    }
    else if (mp->next) {
        mp->next->prev = NULL;
    }

    free(mp->namespace);
    free(mp->name);

    free(mp->metadata->album_artist);
    free(mp->metadata->composer);
    free(mp->metadata->genre);
    free(mp->metadata->artist);
    free(mp->metadata->comment);
    free(mp->metadata->track_id);
    free(mp->metadata->album);
    free(mp->metadata->content_created);
    free(mp->metadata->title);
    free(mp->metadata->url);
    free(mp->metadata->art_url);
    free(mp->metadata);

    free(mp->properties->loop_status);
    free(mp->properties->playback_status);
    free(mp->properties);

    free(mp->tracklist);

    free(mp);

    return ret;
}

void mpris_player_destroy_all(struct MPRISPlayer *mp)
{
    while (mp != NULL) {
        struct MPRISPlayer *tmp = mp;
        mp = mp->next;
        mpris_player_destroy(tmp);
    }
}

void mpris_player_debug_all(struct MPRISPlayer *mp)
{
    while (mp != NULL) {
        printf("player: %s\n", mp->namespace);
        printf("  name:              %s\n", mp->name);
        printf("  properties\n");
        printf("    volume:          %f\n", mp->properties->volume);
        printf("    position:        %ld\n", mp->properties->position);
        printf("    can_control:     %d\n", mp->properties->can_control);
        printf("    can_go_next:     %d\n", mp->properties->can_go_next);
        printf("    can_go_previous: %d\n", mp->properties->can_go_previous);
        printf("    can_play:        %d\n", mp->properties->can_play);
        printf("    can_pause:       %d\n", mp->properties->can_pause);
        printf("    can_seek:        %d\n", mp->properties->can_seek);
        printf("    shuffle:         %d\n", mp->properties->shuffle);
        printf("    loop_status:     %s\n", mp->properties->loop_status);
        printf("    playback_status: %s\n", mp->properties->playback_status);

        printf("  meta data\n");
        printf("    track_number:    %d\n", mp->metadata->track_number);
        printf("    bitrate:         %d\n", mp->metadata->bitrate);
        printf("    disc_number:     %d\n", mp->metadata->disc_number);
        printf("    length:          %ld\n", mp->metadata->length);
        printf("    album_artist:    %s\n", mp->metadata->album_artist);
        printf("    composer:        %s\n", mp->metadata->composer);
        printf("    genre:           %s\n", mp->metadata->genre);
        printf("    artist:          %s\n", mp->metadata->artist);
        printf("    comment:         %s\n", mp->metadata->comment);
        printf("    track_id:        %s\n", mp->metadata->track_id);
        printf("    album:           %s\n", mp->metadata->album);
        printf("    content_created: %s\n", mp->metadata->content_created);
        printf("    title:           %s\n", mp->metadata->title);
        printf("    url:             %s\n", mp->metadata->url);
        printf("    art_url:         %s\n", mp->metadata->art_url);
        printf("\n");

        mp = mp->next;
    }
}

void mpris_player_debug_all_short(struct MPRISPlayer *mp)
{
    int i=0;
    while (mp != NULL) {
        printf("%d: %s\n", i++, mp->namespace);
        mp = mp->next;
    }
}

void mpris_player_play(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_play_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_pause(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_pause_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_stop(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_stop_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_prev(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_previous_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_next(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_next_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_toggle(char *namespace)
{
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_play_pause_sync(proxy, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_set_position(char *namespace, char *track_id, uint64_t pos)
{
    //"/org/mpris/MediaPlayer2/TrackList/NoTrack"
    mprisMediaPlayer2Player *proxy;
	GError *error;
	error = NULL;
	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);
    mpris_media_player2_player_call_set_position_sync(proxy, track_id, pos, NULL, &error);
	g_object_unref(proxy);
}

void mpris_player_seek(char *namespace, uint64_t pos)
{
}


static struct MPRISPlayer* mpris_player_get_head(struct MPRISPlayer *mp)
{
    while (mp->prev != NULL)
        mp = mp->prev;
    return mp;
}

static struct MPRISPlayer* mpris_player_init(struct MPRISPlayer *prev, char *namespace)
{
    static int id = 0;

    struct MPRISPlayer *mp = malloc(sizeof(struct MPRISPlayer));
    mp->metadata = malloc(sizeof(struct MPRISMetaData));
    mp->properties = malloc(sizeof(struct MPRISProperties));
    mp->tracklist = malloc(sizeof(struct MPRISTrackList));

    mp->next = NULL;
    mp->prev = NULL;
    mp->head = NULL;

    if (prev) {
        prev->next = mp;
        mp->prev = prev;
    }

    mp->id = id++;
    mp->namespace = strdup(namespace);
    mp->name = strdup(MPRIS_METADATA_DEFAULT_STR);

    mp->metadata->track_number = MPRIS_METADATA_DEFAULT_UINT;
    mp->metadata->bitrate = MPRIS_METADATA_DEFAULT_UINT;
    mp->metadata->disc_number = MPRIS_METADATA_DEFAULT_UINT;
    mp->metadata->length = MPRIS_METADATA_DEFAULT_UINT;

    mp->metadata->album_artist = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->composer = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->genre = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->artist = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->comment = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->track_id = strdup("/");
    mp->metadata->album = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->content_created = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->title = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->metadata->url = strdup("");
    mp->metadata->art_url = strdup("");

    mp->properties->loop_status = strdup(MPRIS_METADATA_DEFAULT_STR);
    mp->properties->playback_status = NULL;
    mp->properties->status = MPRIS_STATUS_UNKNOWN;

    mp->properties->position = MPRIS_METADATA_DEFAULT_UINT;
    mp->properties->volume = MPRIS_METADATA_DEFAULT_INT;

    return mp;
}

static int get_player_namespaces(char **buf, int max_players)
{
    GDBusProxy *proxy = NULL;
    GVariant * reply = NULL;
	GError *error = NULL;
    GVariant *temp = NULL;
    int pos = 0;
    int ret = 0;

    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL, /* GDBusInterfaceInfo */
                                           DBUS_DESTINATION,
                                           DBUS_PATH,
                                           DBUS_INTERFACE,
                                           NULL, /* GCancellable */
                                           &error);

    if (proxy == NULL) {
        printf("Error creating proxy\n%s\n", error->message);
        ret = -1;
        goto cleanup;
    }

    reply = g_dbus_proxy_call_sync(proxy,
                                           DBUS_METHOD_LIST_NAMES,
                                           NULL,
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);

    if (reply == NULL) {
        printf("No return value\n%s\n", error->message);
        ret = -1;
        goto cleanup;
    }

    GVariantIter iter;
    GVariant *child;
    temp = g_variant_get_child_value(reply, 0);

    g_variant_iter_init (&iter, temp);
    while ((child = g_variant_iter_next_value (&iter))) {
        if (pos == max_players)
            break;

        if (strstr(g_variant_get_string(child, NULL), MPRIS_PLAYER_NAMESPACE) != NULL)
            buf[pos++] = strdup(g_variant_get_string(child, NULL));

        g_variant_unref (child);
    }

cleanup:
    buf[pos] = NULL;

    if (reply)
        g_variant_unref(reply);
    if (temp)
        g_variant_unref(temp);
    if (proxy)
        g_object_unref(proxy);

    return ret;

}

static int mpris_player_set_string(char **buf, const char *value, const char *default_value)
{
    /* check if a new value should be set or default */
    if (*buf != NULL)
        free(*buf);

    if (value == NULL || strlen(value) == 0)
        *buf = strdup(default_value);
    else
        *buf = strdup(value);

    return 0;
}

static void mpris_player_set_string_from_gvariant(char **buf, GVariant *value, const char *default_value)
{
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_ARRAY)) {
        GVariantIter iter;
        GVariant *child;
        char strbuf[256] = "";

        g_variant_iter_init (&iter, value);
        while ((child = g_variant_iter_next_value (&iter))) {
            strncat(strbuf, g_variant_get_string(child, NULL), strlen(g_variant_get_string(child, NULL)));
            g_variant_unref (child);
        }
        mpris_player_set_string(buf, strbuf, default_value);
    }

    else if (!(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING) || g_variant_is_of_type(value, G_VARIANT_TYPE_OBJECT_PATH))) {
        mpris_player_set_string(buf, NULL, default_value);
    }

    else {
        mpris_player_set_string(buf, g_variant_get_string(value, NULL), default_value);
    }
}

static int mpris_player_get_identity(char *namespace, char **buf)
{
    int ret = 0;
    mprisMediaPlayer2 *proxy = NULL;
	GError *error = NULL;

	proxy = mpris_media_player2_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       namespace,
                                                       MPRIS_PLAYER_OBJECT_PATH,
                                                       NULL,
                                                       &error);

    if (proxy == NULL) {
        printf("Error creating proxy\n%s\n", error->message);
        ret = -1;
        goto cleanup;
    }

    mpris_player_set_string(buf, mpris_media_player2_get_identity(proxy), MPRIS_METADATA_DEFAULT_STR);

cleanup:
    if (proxy)
        g_object_unref(proxy);
    return ret;
}

static int mpris_player_load(struct MPRISPlayer *mp)
{
    int ret = 0;
    mprisMediaPlayer2Player *proxy = NULL;
    GVariant *metadata = NULL;
	GError *error = NULL;

	proxy = mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              mp->namespace,
                                                              MPRIS_PLAYER_OBJECT_PATH,
                                                              NULL,
                                                              &error);


    if (proxy == NULL) {
        printf("Error creating proxy\n%s\n", error->message);
        ret = -1;
        goto cleanup;
    }

    mpris_player_set_string(&(mp->properties->loop_status),     mpris_media_player2_player_get_loop_status(proxy), MPRIS_METADATA_DEFAULT_STR);
    mpris_player_set_string(&(mp->properties->playback_status), mpris_media_player2_player_get_playback_status(proxy), MPRIS_METADATA_DEFAULT_STR);
    mpris_player_get_identity(mp->namespace, &(mp->name));

    mp->properties->volume          = mpris_media_player2_player_get_volume(proxy);
    mp->properties->position        = mpris_media_player2_player_get_position(proxy);
    mp->properties->can_control     = mpris_media_player2_player_get_can_control(proxy);
    mp->properties->can_go_next     = mpris_media_player2_player_get_can_go_next(proxy);
    mp->properties->can_go_previous = mpris_media_player2_player_get_can_go_previous(proxy);
    mp->properties->can_play        = mpris_media_player2_player_get_can_play(proxy);
    mp->properties->can_pause       = mpris_media_player2_player_get_can_pause(proxy);
    mp->properties->can_seek        = mpris_media_player2_player_get_can_seek(proxy);
    mp->properties->shuffle         = mpris_media_player2_player_get_shuffle(proxy);

    if (strcmp(mp->properties->playback_status, "Playing") == 0)
        mp->properties->status = MPRIS_STATUS_PLAYING;
    else if (strcmp(mp->properties->playback_status, "Paused") == 0)
        mp->properties->status = MPRIS_STATUS_PAUSED;
    else if (strcmp(mp->properties->playback_status, "Stopped") == 0)
        mp->properties->status = MPRIS_STATUS_STOPPED;
    else
        mp->properties->status = MPRIS_STATUS_UNKNOWN;

    metadata = mpris_media_player2_player_get_metadata(proxy);
    if (metadata == NULL) {
        fprintf(stderr, "Error getting metadata for: %s\n", mp->namespace);
        ret = -1;
        goto cleanup;
    }

    //GVariant *temp = g_variant_get_child_value(metadata, 0);
    GVariantIter iter;
    g_variant_iter_init(&iter, metadata);

    const char *key = NULL;
    GVariant *value = NULL;
    while (g_variant_iter_loop(&iter, "{s@v}", &key, &value)) {
        GVariant *unboxed = g_variant_get_variant(value);

        //printf("k:v  %s => %s\n", key, g_variant_get_type_string(unboxed));

        // strings
        if (strcmp(key, MPRIS_METADATA_ALBUM) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->album), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_TITLE) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->title), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_ART_URL) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->art_url), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_COMMENT) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->comment), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_TRACKID) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->track_id), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_URL) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->url), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_CONTENT_CREATED) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->content_created), unboxed, MPRIS_METADATA_DEFAULT_STR);

        // arrays
        if (strcmp(key, MPRIS_METADATA_ARTIST) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->artist), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_ALBUM_ARTIST) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->album_artist), unboxed, MPRIS_METADATA_DEFAULT_STR);
        if (strcmp(key, MPRIS_METADATA_GENRE) == 0)
            mpris_player_set_string_from_gvariant(&(mp->metadata->genre), unboxed, MPRIS_METADATA_DEFAULT_STR);

        // numbers
        if (strcmp(key, MPRIS_METADATA_TRACK_NUMBER) == 0)
            mp->metadata->track_number = g_variant_get_int32(unboxed);
        if (strcmp(key, MPRIS_METADATA_BITRATE) == 0)
            mp->metadata->bitrate = g_variant_get_int32(unboxed);
        if (strcmp(key, MPRIS_METADATA_DISC_NUMBER) == 0)
            mp->metadata->disc_number = g_variant_get_int32(unboxed);
        if (strcmp(key, MPRIS_METADATA_LENGTH) == 0)
            mp->metadata->length = g_variant_get_int64(unboxed);
            // set int
    }


    //g_variant_iter_init (&iter, temp);
    //while ((child = g_variant_iter_next_value (&iter))) {
    //    if (g_variant_is_of_type(child, G_VARIANT_TYPE_STRING))
    //        printf("item: %s\n", g_variant_get_string(child, NULL));
    //    else
    //        printf("type: %s\n",g_variant_get_type_string(child));


    //    g_variant_unref (child);
    //}

cleanup:
    if (proxy)
        g_object_unref(proxy);
    //if (metadata)
    //    g_variant_unref(metadata);
    //if (temp)
    //    g_variant_unref(temp);
    return ret;
}

