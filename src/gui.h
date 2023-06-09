#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>

int start_gui(int argc, char **argv);
static void gui_on_exit(GtkWindow *window);

#endif // !GUI_H
