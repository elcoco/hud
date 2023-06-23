#ifndef MPRIS_H
#define MPRIS_H

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

DBusMessage* dbus_call_method(const char *destination, const char *path, const char *interface, const char *method, char **args);
int mpris_player_load_identity(struct MprisPlayer *mp);
void mpris_player_load_metadata(struct MprisPlayer *mp, DBusMessageIter *iter);
int mpris_player_load_properties(struct MprisPlayer *mp);
struct MprisPlayer* mpris_players_load();
int mpris_play(struct MprisPlayer *mp);
int mpris_pause(struct MprisPlayer *mp);
int mpris_stop(struct MprisPlayer *mp);
int mpris_prev(struct MprisPlayer *mp);
int mpris_next(struct MprisPlayer *mp);
int mpris_toggle(struct MprisPlayer *mp);

#endif // !MPRIS_H
