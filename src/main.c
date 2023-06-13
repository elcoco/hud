#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "notifications_gui.h"
#include "apps_gui.h"


#define RIPGREP_BIN_PATH "/usr/bin/rg"
#define RIPGREP_ARGS     "--json"
//#define RIPGREP_ARGS     "-l --color never"
#define RIPGREP_PWD      "~"
#define RIPGREP_SEARCH   "next"

#define MAXBUF 1024 * 10



void die(char *fmt, ...)
{
    va_list ptr;
    va_start(ptr, fmt);
    vfprintf(stderr, fmt, ptr);
    va_end(ptr);
    exit(1);
}

static void app_activate(GtkApplication *app)
{
    GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");
    GObject *win = gtk_builder_get_object(builder, "win");
    GObject *w_stack = gtk_builder_get_object(builder, "stack");
    GObject *w_search_entry = gtk_builder_get_object(builder, "search_entry");

    gtk_window_set_application(GTK_WINDOW(win), app);


    // init notification tab
    //GObject *w_notifications = notifications_gui_init(builder, w_search_entry);
    //GtkStackPage *w_stackpage = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(w_notifications));
    //gtk_stack_page_set_title(w_stackpage, "notifications");
    //gtk_stack_page_set_name(w_stackpage, "notifications");
    //gtk_stack_set_visible_child_name(GTK_STACK(w_stack), "notifications");

    GObject *w_apps = apps_gui_init(w_search_entry);
    GtkStackPage *w_stackpage_apps = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(w_apps));
    gtk_stack_page_set_title(w_stackpage_apps, "Apps");
    gtk_stack_page_set_name(w_stackpage_apps, "apps");
    //gtk_stack_set_visible_child_name(GTK_STACK(w_stack), "notifications");


    gtk_window_present(GTK_WINDOW(win));
    g_object_unref(builder);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("com.eco.hud", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    int stat = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return stat;

    return 0;
}
