#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "notifications_gui.h"
#include "apps_gui.h"
#include "search_gui.h"
#include "dashboard_gui.h"
#include "mpris_gui.h"
#include "dock_gui.h"

//#include "sock.h"
#include "state.h"
#include "module.h"
#include "config.h"
#include "utils.h"
//#include "pulse.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#define MAX_PATH_LEN 64

// https://book.huihoo.com/gtk+-gnome-application-development/sec-containers.html


// main maindow
GObject *win = NULL;

// listener thread that handles window visibillity
pthread_t thread_id;
//struct ThreadArgs ta;

struct State state;

// store all modules here
struct Module *m;

static gboolean on_switch_stackpage_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    gtk_stack_set_visible_child_name(GTK_STACK(widget), (char*)data);
    return 1;
}

static gint on_glist_find_custom_cb(gconstpointer a, gconstpointer b)
{
    /* Custom callback for finding stuff in a glist linked list */
    if (strcmp(a, b) == 0)
        return 0;
    else
        return 1;
}

static gboolean on_focus_tab_prev_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    const char *cur = gtk_stack_get_visible_child_name(GTK_STACK(widget));

    GList *first = data;
    GList *match;

    if ((match = g_list_find_custom(first, cur, on_glist_find_custom_cb))) {
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

static gboolean on_focus_tab_next_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    const char *cur = gtk_stack_get_visible_child_name(GTK_STACK(widget));

    GList *first = data;
    GList *match;

    if ((match = g_list_find_custom(first, cur, on_glist_find_custom_cb))) {
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

static int set_win_visible(GtkWindow *win, struct State *s)
{
    DEBUG("Set visible\n");
    gtk_widget_set_visible(GTK_WIDGET(win), 1);
    if (s->fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(win));
    return 0;
}

static int set_win_invisible(GtkWindow *win)
{
    DEBUG("Set invisible\n");
    gtk_widget_set_visible(GTK_WIDGET(win), 0);
    return 0;
}

static void setup_keys(GtkWidget *widget)
{
    // keep track of stack page names so we can cycle them
    // not going to free this, is needed for callbacks and won't change
    GList *names = NULL;

    // setup keyboard shortcuts
    GtkEventController *controller = gtk_shortcut_controller_new();
    gtk_widget_add_controller(GTK_WIDGET(widget), controller);

    GListModel *m = G_LIST_MODEL(gtk_stack_get_pages(GTK_STACK(widget)));
    for (int i=0 ; i<g_list_model_get_n_items(m) ; i++) {
        GtkStackPage *p = g_list_model_get_item(m, i);
        char *name = (char*)gtk_stack_page_get_name(p);

        if (names == NULL)
            names = g_list_append(NULL, strdup(name));
        else
            names = g_list_append(names, strdup(name));

        if (i <= 9) {
            gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                                 gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_1 + i, GDK_ALT_MASK),
                                                                  gtk_callback_action_new(on_switch_stackpage_cb, name, NULL)));
        }
    }

    // cycle through stack pages using ctrl-tab / ctrl-shift-tab
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_Tab, GDK_CONTROL_MASK),
                                                          gtk_callback_action_new(on_focus_tab_next_cb, names, NULL)));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
                                         gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_Tab, GDK_SHIFT_MASK | GDK_CONTROL_MASK),
                                                          gtk_callback_action_new(on_focus_tab_prev_cb, names, NULL)));
}

static gboolean on_stack_moved_focus_cb(GtkWidget *widget, GObject *pspec, gpointer data)
{
    /* lock all modules except for the visible one */
    const char *name = gtk_stack_get_visible_child_name(GTK_STACK(widget));
    struct Module *m = data;

    while (m != NULL) {
        if (strcmp(m->name, name) == 0)
            module_unlock(m);
        else
            module_lock(m);
        m = m->next;
    }

    return 1;
}

static gboolean on_mod_exit(GtkWidget *widget, GObject *pspec, gpointer data)
{
    DEBUG("received exit!!!!!!\n");
    if (state.hide_on_close)
        set_win_invisible(GTK_WINDOW(win));
    else
        g_application_quit(G_APPLICATION(data));
    return 1;
}

static void app_activate_cb(GtkApplication *app, struct Config *c)
{
    // when seconds instance is started, GtkApplication emits activate signal wo
    // we need to catch that here
    if (win != NULL) {
        set_win_visible(GTK_WINDOW(win), &state);
        return;
    }
    
    GtkBuilder *builder = gtk_builder_new_from_resource("/resources/ui/gui.ui");
    win = gtk_builder_get_object(builder, "main_win");
    GObject *w_stack = gtk_builder_get_object(builder, "main_stack");
    GObject *main_box = gtk_builder_get_object(builder, "main_box");

    if (state.fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(win));

    g_signal_new("module-exit",
             G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
             0, NULL, NULL,
             g_cclosure_marshal_VOID__POINTER,
             G_TYPE_NONE, 1, G_TYPE_POINTER);

    // signal is used by modules to signal application exit, for example when executing an app
    g_signal_connect(G_OBJECT(win), "module-exit", G_CALLBACK(on_mod_exit), app);

    gtk_window_set_application(GTK_WINDOW(win), app);

    if (state.hide_on_close)
        gtk_window_set_hide_on_close(GTK_WINDOW(win), TRUE);

    m = module_init(NULL, "apps",          c, apps_gui_init, NULL);
    m = module_init(m,    "dock",          c, dock_gui_init, NULL);
    m = module_init(m,    "notifications", c, notifications_gui_init, NULL);
    m = module_init(m,    "search",        c, search_gui_init, NULL);
    m = module_init(m,    "mpris",         c, mpris_gui_init, NULL);
//    m = module_init(m,    "dashboard",     dashboard_gui_init, NULL);

    struct Module *tmp = m->head;

    while (tmp != NULL) {
        // move to initializer
        tmp->main_win = win;
        module_set_main_win(m, win);

        module_activate(tmp);

        if (strcmp(tmp->name, "dock") == 0) {
            gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(tmp->widget));
        }
        else {
            GtkStackPage *page = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(tmp->widget));
            gtk_stack_page_set_title(page, tmp->name);
            gtk_stack_page_set_name(page, tmp->name);
        }
        tmp = tmp->next;
    }
    
    // lock all modules except for visible one (lock threads/running processes)
    on_stack_moved_focus_cb(GTK_WIDGET(w_stack), NULL, m->head);
    g_signal_connect(GTK_STACK(w_stack), "notify::visible-child", G_CALLBACK(on_stack_moved_focus_cb), m->head);

    module_debug(m);

    //gtk_stack_set_visible_child_name(GTK_STACK(w_stack), state.focus_page);
    //GtkRecentManager *manager = gtk_recent_manager_get_default ();

    setup_keys(GTK_WIDGET(w_stack));

    gtk_window_present(GTK_WINDOW(win));
    g_object_unref(builder);
}

static void show_help()
{
    INFO("HUD :: Hud stuff\n");
    INFO("Optional args:\n");
    INFO("  -f <name>     Focus page at start\n");
    INFO("  -H            Hide window instead of close\n");
    INFO("  -D            Debug\n");
    INFO("  -h            Show help\n");
}

static int parse_args(struct State *s, int argc, char **argv)
{
    int option;
    DEBUG("Parsing args\n");

    while((option = getopt(argc, argv, "f:HhDF")) != -1) {
        switch (option) {
            case 'f':
                strcpy(s->focus_page, optarg);
                break;
            case 'F':
                s->fullscreen = 1;
                break;
            case 'H':
                s->hide_on_close = 1;
                break;
            case 'D': 
                s->do_debug = 1;
                break;
            case ':': 
                ERROR("Option needs a value\n"); 
                return -1;
            case 'h': 
                show_help();
                return -1;
            case '?': 
                show_help();
                return -1;
       }
    }
    return 1;
}

static void init_state(struct State *s)
{
    /* Set defaults, can be overridden in parse_args() */
    s->hide_on_close = 0;
    s->do_debug = 1;
    s->fullscreen = 0;

    strcpy(s->focus_page, "apps");
}

static void config_write_defaults(struct Config *c)
{
    config_set_number(c, "core", "fullscreen", 1);
}

int main(int argc, char **argv)
{


    // TODO if socket already up, connect to running session
    char path[256] = "";
    char *dir;
    char filename[MAX_PATH_LEN] = "";

    if ((dir = getenv("XDG_CONFIG_HOME")) == NULL) {
        if ((dir = getenv("HOME")) == NULL) {
            ERROR("$XDG_CONFIG_HOME and $HOME not set\n");
            return -1;
        }
        else {
            strncpy(filename, ".hud.json", MAX_PATH_LEN);
        }
    }
    else {
        strncpy(filename, "hud.json", MAX_PATH_LEN);
    }

    sprintf(path, "%s/%s", dir, filename);
    struct Config *c = config_init(path);
    if (config_file_exists(c) < CONFIG_SUCCESS) {
        INFO("Writing config defaults to: %s\n", c->path);
        if (config_file_create(c) < CONFIG_SUCCESS)
            return 0;
        config_write_defaults(c);
    }

    init_state(&state);
    if (parse_args(&state, argc, argv) < 0)
        return 1;

    GtkApplication *app = gtk_application_new("com.eco.hud", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate_cb), c);

    int stat = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return stat;
}
