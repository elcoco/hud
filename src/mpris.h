#ifndef MPRIS_H
#define MPRIS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "mpris_gdbus.h"

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

#define MPRIS_METADATA_BITRATE      "bitrate"
#define MPRIS_METADATA_ART_URL      "mpris:artUrl"
#define MPRIS_METADATA_LENGTH       "mpris:length"
#define MPRIS_METADATA_TRACKID      "mpris:trackid"

#define MPRIS_METADATA_ALBUM        "xesam:album"
#define MPRIS_METADATA_ALBUM_ARTIST "xesam:albumArtist"
#define MPRIS_METADATA_ARTIST       "xesam:artist"
#define MPRIS_METADATA_COMMENT      "xesam:comment"
#define MPRIS_METADATA_COMPOSER     "xesam:composer"
#define MPRIS_METADATA_DISC_NUMBER  "xesam:discNumber"
#define MPRIS_METADATA_GENRE        "xesam:genre"
#define MPRIS_METADATA_CONTENT_CREATED "xesam:contentCreated"
#define MPRIS_METADATA_TITLE        "xesam:title"
#define MPRIS_METADATA_TRACK_NUMBER "xesam:trackNumber"
#define MPRIS_METADATA_URL          "xesam:url"
#define MPRIS_METADATA_YEAR         "year"

#define MPRIS_METADATA_DEFAULT_STR  "Unknown"
#define MPRIS_METADATA_DEFAULT_INT  -1
#define MPRIS_METADATA_DEFAULT_UINT  0

#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"

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
    char *album;
    char *album_artist;
    char *art_url;
    char *artist;
    char *composer;
    char *comment;
    char *content_created;
    char *genre;
    char *title;
    char *track_id;
    char *url;
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
    char *loop_status;
    char *playback_status;
    enum PlayerStatus status;
};

struct MPRISTrackList {
};

struct MPRISPlayer {
    int id;
    char *name;

    struct MPRISPlayer *head;
    struct MPRISPlayer *prev;
    struct MPRISPlayer *next;

    char *namespace;
    struct MPRISMetaData *metadata;
    struct MPRISProperties *properties;
    struct MPRISTrackList *tracklist;
};

struct MPRISPlayer* mpris_player_destroy(struct MPRISPlayer *mp);
void mpris_player_destroy_all(struct MPRISPlayer *mp);
void mpris_player_debug_all(struct MPRISPlayer *mp);
void mpris_player_debug_all_short(struct MPRISPlayer *mp);
//int mpris_player_load_all();
struct MPRISPlayer* mpris_player_load_all(struct MPRISPlayer *mp_old);

void mpris_player_play(char *namespace);
void mpris_player_pause(char *namespace);
void mpris_player_stop(char *namespace);
void mpris_player_prev(char *namespace);
void mpris_player_next(char *namespace);
void mpris_player_toggle(char *namespace);
void mpris_player_set_position(char *namespace, char *track_id, uint64_t pos);
void mpris_player_seek(char *namespace, uint64_t pos);
    
#endif // !MPRIS_H
