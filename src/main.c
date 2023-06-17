#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "notifications_gui.h"
#include "apps_gui.h"
#include "search_gui.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>


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

static gboolean on_shortcut(GtkWidget *widget, GVariant *args, gpointer data)
{
    gtk_stack_set_visible_child_name(GTK_STACK(widget), (char*)data);
    return 1;
}

gint glist_find_custom_cb(gconstpointer a, gconstpointer b)
{
    /* Custom callback for finding stuff in a glist linked list */
    if (strcmp(a, b) == 0)
        return 0;
    else
        return 1;
}

static gboolean focus_tab_prev_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    const char *cur = gtk_stack_get_visible_child_name(GTK_STACK(widget));

    GList *first = data;
    GList *match;

    if ((match = g_list_find_custom(first, cur, glist_find_custom_cb))) {
        GList *prev;

        // wrap
        if (match->prev == NULL)
            prev = g_list_last(first);
        else
            prev = match->prev;
        gtk_stack_set_visible_child_name(GTK_STACK(widget), (char*)prev->data);
    }
    return 1;
}

static gboolean focus_tab_next_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    const char *cur = gtk_stack_get_visible_child_name(GTK_STACK(widget));

    GList *first = data;
    GList *match;

    if ((match = g_list_find_custom(first, cur, glist_find_custom_cb))) {
        GList *next;

        // wrap
        if (match->next == NULL)
            next = first;
        else
            next = match->next;
        gtk_stack_set_visible_child_name(GTK_STACK(widget), (char*)next->data);
    }
    return 1;
}

static gboolean event_key_pressed_cb(GtkWidget       *drawing_area,
                                     guint           keyval,
                                     guint           keycode,
                                     GdkModifierType state,
                                     gpointer        user_data)
{
    printf("keypress\n");
    return FALSE;
}

static void app_activate(GtkApplication *app)
{
    //GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");
    GtkBuilder *builder = gtk_builder_new_from_resource("/resources/ui/gui.ui");
    GObject *win = gtk_builder_get_object(builder, "main_win");
    GObject *w_stack = gtk_builder_get_object(builder, "main_stack");

    gtk_window_set_application(GTK_WINDOW(win), app);

    // keep track of stack page names so we can cycle them
    GList *names = g_list_alloc();
    names = g_list_append(NULL, "apps");
    names = g_list_append(names, "notifications");
    names = g_list_append(names, "search");

    // TOOD do lazy loading of stackpages to save startup time

    // setup apps stackpage
    GObject *w_apps = apps_gui_init();
    GtkStackPage *w_stackpage_apps = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(w_apps));
    gtk_stack_page_set_title(w_stackpage_apps, "Apps");
    gtk_stack_page_set_name(w_stackpage_apps, "apps");
    
    // setup notification stackpage
    GObject *w_notifications = notifications_gui_init();
    GtkStackPage *w_stackpage_notifications = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(w_notifications));
    gtk_stack_page_set_title(w_stackpage_notifications, "Notifications");
    gtk_stack_page_set_name(w_stackpage_notifications, "notifications");

    GObject *w_search = search_gui_init();
    GtkStackPage *w_stackpage_search = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(w_search));
    gtk_stack_page_set_title(w_stackpage_search, "Search");
    gtk_stack_page_set_name(w_stackpage_search, "search");

    //gtk_stack_set_visible_child_name(GTK_STACK(w_stack), "search");

    // setup keyboard shortcuts
    GtkEventController *controller = gtk_shortcut_controller_new();
    gtk_widget_add_controller(GTK_WIDGET(w_stack), controller);
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_1, GDK_ALT_MASK),
                                                          gtk_callback_action_new(on_shortcut, "apps", NULL)));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_2, GDK_ALT_MASK),
                                                          gtk_callback_action_new(on_shortcut, "notifications", NULL)));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_3, GDK_ALT_MASK),
                                                          gtk_callback_action_new(on_shortcut, "bever", NULL)));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_Escape, 0),
                                                          gtk_callback_action_new(on_shortcut, "bever", NULL)));

    // cycle through stack pages using ctrl-tab / ctrl-shift-tab
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_Tab, GDK_CONTROL_MASK),
                                                          gtk_callback_action_new(focus_tab_next_cb, names, NULL)));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_Tab, GDK_SHIFT_MASK | GDK_CONTROL_MASK),
                                                          gtk_callback_action_new(focus_tab_prev_cb, names, NULL)));



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
}
