#ifndef MPRIS_H
#define MPRIS_H

// thanks mariusor! => https://github.com/mariusor/mpris-ctl
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include <dbus/dbus.h>
#include <glib.h>
#include <gio/gio.h>

#include "mpris_gdbus.h"

// custom assert macro with formatted message and ui cleanup
#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

#define MPRIS_PLAYER_NAMESPACE     "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_OBJECT_PATH   "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"

#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"
#define MPRIS_METHOD_SEEK          "Seek"
#define MPRIS_METHOD_SET_POSITION  "SetPosition"

#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define MPRIS_PROP_PLAYBACKSTATUS "PlaybackStatus"
#define MPRIS_PROP_CANCONTROL     "CanControl"
#define MPRIS_PROP_CANGONEXT      "CanGoNext"
#define MPRIS_PROP_CANGOPREVIOUS  "CanGoPrevious"
#define MPRIS_PROP_CANPLAY        "CanPlay"
#define MPRIS_PROP_CANPAUSE       "CanPause"
#define MPRIS_PROP_CANSEEK        "CanSeek"
#define MPRIS_PROP_SHUFFLE        "Shuffle"
#define MPRIS_PROP_POSITION       "Position"
#define MPRIS_PROP_VOLUME         "Volume"
#define MPRIS_PROP_LOOPSTATUS     "LoopStatus"
#define MPRIS_PROP_METADATA       "Metadata"

 
#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"
#define DBUS_METHOD_SET            "Set"

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

enum PlayerStatus {
    MPRIS_STATUS_PLAYING,
    MPRIS_STATUS_STOPPED,
    MPRIS_STATUS_PAUSED,
    MPRIS_STATUS_UNKNOWN
};

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
    enum PlayerStatus status;
};

struct MprisPlayer {
    int id;
    struct MprisPlayer *next;
    char *namespace;
    struct MPRISMetaData metadata;
    struct MPRISProperties properties;
};


struct MprisPlayer* mpris_player_load_all();

void mpris_player_play(char *namespace);
void mpris_player_pause(char *namespace);
void mpris_player_stop(char *namespace);
void mpris_player_prev(char *namespace);
void mpris_player_next(char *namespace);
void mpris_player_toggle(char *namespace);
void mpris_player_set_position(char *namespace, char *track_id, uint64_t pos);
void mpris_player_seek(char *namespace, uint64_t pos);

void mpris_player_debug_all(struct MprisPlayer *mp);
void mpris_player_destroy_all(struct MprisPlayer *mp);

#endif // !MPRIS_H
