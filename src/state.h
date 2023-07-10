#ifndef STATE_H
#define STATE_H

struct State {
    int hide_on_close;
    char focus_page[256];
    int do_debug;
    int fullscreen;
};

#endif // !STATE_H
