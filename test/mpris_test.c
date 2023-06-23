// compile:
// gcc -o dbus -Wall dbus.c `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1
//
// thanks mariusor! => https://github.com/mariusor/mpris-ctl
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <dbus/dbus.h>


#define MPRIS_PLAYER_NAMESPACE     "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"

#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define MPRIS_PNAME_PLAYBACKSTATUS "PlaybackStatus"
#define MPRIS_PNAME_CANCONTROL     "CanControl"
#define MPRIS_PNAME_CANGONEXT      "CanGoNext"
#define MPRIS_PNAME_CANGOPREVIOUS  "CanGoPrevious"
#define MPRIS_PNAME_CANPLAY        "CanPlay"
#define MPRIS_PNAME_CANPAUSE       "CanPause"
#define MPRIS_PNAME_CANSEEK        "CanSeek"
#define MPRIS_PNAME_SHUFFLE        "Shuffle"
#define MPRIS_PNAME_POSITION       "Position"
#define MPRIS_PNAME_VOLUME         "Volume"
#define MPRIS_PNAME_LOOPSTATUS     "LoopStatus"
#define MPRIS_PNAME_METADATA       "Metadata"

 
#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"

#define MPRIS_METADATA_BITRATE      "bitrate"
#define MPRIS_METADATA_ART_URL      "mpris:artUrl"
#define MPRIS_METADATA_LENGTH       "mpris:length"
#define MPRIS_METADATA_TRACKID      "mpris:trackid"
#define MPRIS_METADATA_ALBUM        "xesam:album"
#define MPRIS_METADATA_ALBUM_ARTIST "xesam:albumArtist"
#define MPRIS_METADATA_ARTIST       "xesam:artist"
#define MPRIS_METADATA_COMMENT      "xesam:comment"
#define MPRIS_METADATA_TITLE        "xesam:title"
#define MPRIS_METADATA_TRACK_NUMBER "xesam:trackNumber"
#define MPRIS_METADATA_URL          "xesam:url"
#define MPRIS_METADATA_YEAR         "year"

#define MPRIS_METADATA_VALUE_STOPPED "Stopped"
#define MPRIS_METADATA_VALUE_PLAYING "Playing"
#define MPRIS_METADATA_VALUE_PAUSED  "Paused"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms
                                       //
#define MAX_PLAYERS 20

struct MPRISMetaData {
    uint64_t length; // mpris specific
    unsigned short track_number;
    unsigned short bitrate;
    unsigned short disc_number;
    char *album_artist;
    char *composer;
    char *genre;
    char *artist;
    char *comment;
    char *track_id;
    char *album;
    char *content_created;
    char *title;
    char *url;
    char *art_url;
};

struct MPRISProperties {
    double volume;
    uint64_t position;
    int can_control;
    int can_go_next;
    int can_go_previous;
    int can_play;
    int can_pause;
    int can_seek;
    int shuffle;
    char *player_name;
    char *loop_status;
    char *playback_status;
};

struct MprisPlayer {
    struct MprisPlayer *next;
    char *namespace;
    struct MPRISMetaData metadata;
    struct MPRISProperties properties;
};


static struct MprisPlayer* mpris_player_init(struct MprisPlayer *prev, char *namespace)
{
    struct MprisPlayer *mp = malloc(sizeof(struct MprisPlayer));
    mp->next = NULL;
    if (prev)
        prev->next = mp;

    mp->namespace = strdup(namespace);

    mp->metadata.track_number = 0;
    mp->metadata.bitrate = 0;
    mp->metadata.disc_number = 0;
    mp->metadata.length = 0;

    mp->metadata.album_artist = strdup("unknown");
    mp->metadata.composer = strdup("unknown");
    mp->metadata.genre = strdup("unknown");
    mp->metadata.artist = strdup("unknown");
    mp->metadata.comment = NULL;
    mp->metadata.track_id = NULL;
    mp->metadata.album = strdup("unknown");
    mp->metadata.content_created = NULL;
    mp->metadata.title = strdup("unknown");
    mp->metadata.url = NULL;
    mp->metadata.art_url = NULL;

    mp->properties.player_name = NULL;
    mp->properties.loop_status = NULL;
    mp->properties.playback_status = NULL;

    return mp;
}

void mpris_destroy(struct MprisPlayer *mp)
{
    free(mp->namespace);

    free(mp->metadata.album_artist);
    free(mp->metadata.composer);
    free(mp->metadata.genre);
    free(mp->metadata.artist);
    free(mp->metadata.comment);
    free(mp->metadata.track_id);
    free(mp->metadata.album);
    free(mp->metadata.content_created);
    free(mp->metadata.title);
    free(mp->metadata.url);
    free(mp->metadata.art_url);

    free(mp->properties.player_name);
    free(mp->properties.loop_status);
    free(mp->properties.playback_status);

    free(mp);
}

void mpris_players_debug(struct MprisPlayer *mp)
{
    while (mp != NULL) {
        printf("player: %s\n", mp->namespace);
        printf("  properties\n");
        printf("    volume:          %f\n", mp->properties.volume);
        printf("    position:        %ld\n", mp->properties.position);
        printf("    can_control:     %d\n", mp->properties.can_control);
        printf("    can_go_next:     %d\n", mp->properties.can_go_next);
        printf("    can_go_previous: %d\n", mp->properties.can_go_previous);
        printf("    can_play:        %d\n", mp->properties.can_play);
        printf("    can_pause:       %d\n", mp->properties.can_pause);
        printf("    can_seek:        %d\n", mp->properties.can_seek);
        printf("    shuffle:         %d\n", mp->properties.shuffle);
        printf("    player_name:     %s\n", mp->properties.player_name);
        printf("    loop_status:     %s\n", mp->properties.loop_status);
        printf("    playback_status: %s\n", mp->properties.playback_status);

        printf("  meta data\n");
        printf("    track_number:    %d\n", mp->metadata.track_number);
        printf("    bitrate:         %d\n", mp->metadata.bitrate);
        printf("    disc_number:     %d\n", mp->metadata.disc_number);
        printf("    length:          %ld\n", mp->metadata.length);
        printf("    album_artist:    %s\n", mp->metadata.album_artist);
        printf("    composer:        %s\n", mp->metadata.composer);
        printf("    genre:           %s\n", mp->metadata.genre);
        printf("    artist:          %s\n", mp->metadata.artist);
        printf("    comment:         %s\n", mp->metadata.comment);
        printf("    track_id:        %s\n", mp->metadata.track_id);
        printf("    album:           %s\n", mp->metadata.album);
        printf("    content_created: %s\n", mp->metadata.content_created);
        printf("    title:           %s\n", mp->metadata.title);
        printf("    url:             %s\n", mp->metadata.url);
        printf("    art_url:         %s\n", mp->metadata.art_url);
        printf("\n");

        mp = mp->next;
    }
}

static double extract_double_var(DBusMessageIter *iter, DBusError *error)
{
    double result = 0;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

char* extract_string_var(DBusMessageIter *iter, DBusError *error)
{
    char strbuf[256] = "";

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        //dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        fprintf(stderr, "iter should be variant\n");
        return NULL;
    }

    DBusMessageIter variantIter = {0};
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter)) {
        char *val = NULL;
        dbus_message_iter_get_basic(&variantIter, &val);
        memcpy(strbuf, val, strlen(val));
    } else if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)) {
        char *val = NULL;
        dbus_message_iter_get_basic(&variantIter, &val);
        memcpy(strbuf, val, strlen(val));
    } else if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);
        while (1) {
            // todo(marius): load all elements of the array
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                char *val = NULL;
                dbus_message_iter_get_basic(&arrayIter, &val);
                strncat(strbuf, val, 256 - strlen(strbuf) - 1);
            }
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            }
            dbus_message_iter_next(&arrayIter);
        }
    }
    return strdup(strbuf);
}

static int32_t extract_int32_var(DBusMessageIter *iter, DBusError *error)
{
    int32_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

static int64_t extract_int64_var(DBusMessageIter *iter, DBusError *error)
{
    int64_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    if (DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter)) {
        uint64_t temp;
        dbus_message_iter_get_basic(&variantIter, &temp);
        result = (int64_t)temp;
        return result;
    }
    return 0;
}

static int extract_boolean_var(DBusMessageIter *iter, DBusError *error)
{
    int result = 0;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

DBusMessage* dbus_call_method(const char *destination, const char *path, const char *interface, const char *method, char **args)
{
    /* Call method and return reply message */
    static DBusConnection *conn = NULL;
    DBusError err = {0};
    dbus_error_init(&err);

    if (conn == NULL) {
        conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err)) {
            fprintf(stderr, "DBus connection error(%s)\n", err.message);
            goto cleanup_on_err;
        }
    }

    DBusMessage* msg;
    DBusMessage* reply = NULL;
    DBusPendingCall* pending;
    DBusMessage *ret = NULL;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (msg == NULL) {
        fprintf(stderr, "Failed to make new method call\n");
        goto cleanup_on_err;
    }

    if (args != NULL) {
        char **arg = args;
        while (*arg != NULL) {
            printf("arg: %s\n", *arg);

            DBusMessageIter params;
            // append interface we want to get the property from
            dbus_message_iter_init_append(msg, &params);
            if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, arg)) {
                fprintf(stderr, "Failed to append argument to dbus method call\n");
                goto cleanup_on_err;
            }
            arg++;
        }
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending, DBUS_CONNECTION_TIMEOUT) || pending == NULL) {
        fprintf(stderr, "Failed to send message\n");
        goto cleanup_on_err;
    }

    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    // get the reply message
    if ((reply = dbus_pending_call_steal_reply(pending)) == NULL) {
        fprintf(stderr, "Failed to get reply\n");
        goto cleanup_on_err;
    }

    ret = reply;

cleanup_on_err:
    if (pending)
        dbus_pending_call_unref(pending);
    if (msg)
        dbus_message_unref(msg);
    if (dbus_error_is_set(&err))
        dbus_error_free(&err);
    return ret;
}

int mpris_player_load_identity(struct MprisPlayer *mp)
{
    int ret = 0;
    DBusMessage *reply;
    DBusMessageIter rootiter;
    DBusError err;
    dbus_error_init(&err);

    printf("Loading identity for %s\n", mp->namespace);

    char *args[] = {MPRIS_PLAYER_NAMESPACE, MPRIS_ARG_PLAYER_IDENTITY, NULL};
    if ((reply = dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, DBUS_PROPERTIES_INTERFACE, DBUS_METHOD_GET, args)) == NULL) {
        printf("No reply\n");
        ret = -1;
        goto cleanup_on_err;
    }

    if (!dbus_message_iter_init(reply, &rootiter)) {
        fprintf(stderr, "Failed to get iter init\n");
        ret = -1;
        goto cleanup_on_err;
    }

    mp->properties.player_name = extract_string_var(&rootiter, &err);

cleanup_on_err:
    if (reply)
        dbus_message_unref(reply);
    if (dbus_error_is_set(&err))
        dbus_error_free(&err);

    return ret;
}

void mpris_player_load_metadata(struct MprisPlayer *mp, DBusMessageIter *iter)
{
    DBusError err;
    dbus_error_init(&err);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(&err, "variant_should_be_array", "This variant reply message must have array content");
        return;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    while (1) {
        char* key = NULL;
        if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayIter)) {
            DBusMessageIter dictiter;
            dbus_message_iter_recurse(&arrayIter, &dictiter);
            if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictiter)) {
                dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictiter, &key);

            if (!dbus_message_iter_has_next(&dictiter)) {
                continue;
            }
            dbus_message_iter_next(&dictiter);

            if (!strncmp(key, MPRIS_METADATA_BITRATE, strlen(MPRIS_METADATA_BITRATE))) {
                mp->metadata.bitrate = extract_int32_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                mp->metadata.art_url = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                mp->metadata.length = extract_int64_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                mp->metadata.track_id = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                mp->metadata.album_artist = extract_string_var(&dictiter, &err);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM))) {
                mp->metadata.album = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                mp->metadata.artist = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                mp->metadata.comment = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                mp->metadata.title = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                mp->metadata.track_number = extract_int32_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                mp->metadata.url = extract_string_var(&dictiter, &err);
            }
            if (dbus_error_is_set(&err)) {
                fprintf(stderr, "err: %s, %s\n", key, err.message);
                dbus_error_free(&err);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
}


int mpris_player_load_properties(struct MprisPlayer *mp)
{
    int ret = 0;
    DBusMessage *reply;
    DBusMessageIter rootiter;
    DBusError err;
    dbus_error_init(&err);

    printf("Loading properties for %s\n", mp->namespace);

    char *args[] = {MPRIS_PLAYER_INTERFACE, NULL};
    if ((reply = dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, DBUS_PROPERTIES_INTERFACE, DBUS_METHOD_GET_ALL, args)) == NULL) {
        printf("No reply\n");
        ret = -1;
        goto cleanup_on_err;
    }

    if (!dbus_message_iter_init(reply, &rootiter)) {
        fprintf(stderr, "Failed to get iter init\n");
        ret = -1;
        goto cleanup_on_err;
    }

    if (dbus_message_iter_get_arg_type(&rootiter) != DBUS_TYPE_ARRAY) {
        ret = -1;
        goto cleanup_on_err;
    }

    DBusMessageIter arrayElementIter;
    dbus_message_iter_recurse(&rootiter, &arrayElementIter);

    while (1) {
        char* key;

        if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayElementIter)) {
            DBusMessageIter dictiter;
            dbus_message_iter_recurse(&arrayElementIter, &dictiter);
            if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictiter)) {
                dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictiter, &key);

            printf("Parsing key: %s\n", key);

            if (!dbus_message_iter_has_next(&dictiter)) {
                continue;
            }
            dbus_message_iter_next(&dictiter);

            if (!strncmp(key, MPRIS_PNAME_CANCONTROL, strlen(MPRIS_PNAME_CANCONTROL))) {
                mp->properties.can_control = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                mp->properties.can_go_next = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                mp->properties.can_go_previous = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                mp->properties.can_pause = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                mp->properties.can_play = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                mp->properties.can_seek = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                mp->properties.loop_status = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                mpris_player_load_metadata(mp, &dictiter);
            }
            if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                mp->properties.playback_status = extract_string_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                mp->properties.position= extract_int64_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                mp->properties.shuffle = extract_boolean_var(&dictiter, &err);
            }
            if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                mp->properties.volume = extract_double_var(&dictiter, &err);
            }
        }

        if (!dbus_message_iter_has_next(&arrayElementIter)) {
            break;
        }
        dbus_message_iter_next(&arrayElementIter);
    }

cleanup_on_err:
    if (reply)
        dbus_message_unref(reply);
    if (dbus_error_is_set(&err))
        dbus_error_free(&err);
    return ret;
}

struct MprisPlayer* mpris_players_load()
{
    DBusMessageIter rootiter;
    DBusMessage *reply;
    DBusError err = {0};
    dbus_error_init(&err);

    int cnt = 0;

    struct MprisPlayer *prev = NULL;
    struct MprisPlayer *mp = NULL;
    struct MprisPlayer *mp_head = NULL;
    struct MprisPlayer *ret = NULL;

    if ((reply = dbus_call_method(DBUS_DESTINATION, DBUS_PATH, DBUS_INTERFACE, DBUS_METHOD_LIST_NAMES, NULL)) == NULL) {
        goto cleanup_on_err;
    }

    if (!dbus_message_iter_init(reply, &rootiter)) {
        fprintf(stderr, "Failed to get iter init\n");
        goto cleanup_on_err;
    }

    if (dbus_message_iter_get_arg_type(&rootiter) != DBUS_TYPE_ARRAY)
        return NULL;

    DBusMessageIter arrayElementIter;

    dbus_message_iter_recurse(&rootiter, &arrayElementIter);
    while (cnt < MAX_PLAYERS) {

        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
            char *namespace = NULL;
            dbus_message_iter_get_basic(&arrayElementIter, &namespace);
            if (!strncmp(namespace, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE))) {
                mp = mpris_player_init(prev, namespace);
                cnt++;

                if (prev == NULL)
                    mp_head = mp;

                prev = mp;
            }
        }
        if (!dbus_message_iter_has_next(&arrayElementIter)) {
            break;
        }
        dbus_message_iter_next(&arrayElementIter);
    }
    mp = mp_head;
    while (mp != NULL) {
        mpris_player_load_properties(mp);
        mp = mp->next;
    }

    mp = mp_head;
    while (mp != NULL) {
        mpris_player_load_identity(mp);
        mp = mp->next;
    }

    ret = mp_head;

cleanup_on_err:
    if (reply)
        dbus_message_unref(reply);
    if (dbus_error_is_set(&err))
        dbus_error_free(&err);

    return mp_head;
}

int mpris_play(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_PLAY, NULL);
}

int mpris_pause(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_PAUSE, NULL);
}

int mpris_stop(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_STOP, NULL);
}

int mpris_prev(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_PREVIOUS, NULL);
}

int mpris_next(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_NEXT, NULL);
}

int mpris_toggle(struct MprisPlayer *mp)
{
    dbus_call_method(mp->namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_PLAY_PAUSE, NULL);
}

int main()
{
    struct MprisPlayer *mp_head = mpris_players_load();
    if (mp_head == NULL) {
        printf("Failed to get players\n");
        return -1;
    }

    mpris_toggle(mp_head);

    mpris_players_debug(mp_head);



    return 0;
}
