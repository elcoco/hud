#include "gui.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>


static void bevers(GtkWindow *window)
{
  gtk_window_close(window);
}

static void gui_activate2(GtkApplication *app, gpointer user_data)
{
    /* Construct a GtkBuilder instance and load our UI description */
    //GtkBuilder *builder = gtk_builder_new ();
    //gtk_builder_add_from_file (builder, "gui.ui", NULL);
    GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");

    /* Connect signal handlers to the constructed widgets. */
    GtkWidget *window = gtk_builder_get_object(builder, "win");
    gtk_window_set_application(GTK_WINDOW(window), app);

    gtk_window_present(GTK_WINDOW(window));

    /* We do not need the builder any more */
    g_object_unref(builder);
}

int start_gui(int argc, char *argv[])
{

    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(gui_activate2), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
