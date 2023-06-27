#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "notifications_gui.h"
#include "apps_gui.h"
#include "search_gui.h"
#include "dashboard_gui.h"
#include "mpris_gui.h"

#include "sock.h"
#include "state.h"
#include "module.h"
//#include "pulse.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>


#define RIPGREP_BIN_PATH "/usr/bin/rg"
#define RIPGREP_ARGS     "--json"
//#define RIPGREP_ARGS     "-l --color never"
#define RIPGREP_PWD      "~"
#define RIPGREP_SEARCH   "next"

#define MAXBUF 1024 * 10

// listener thread that handles window visibillity
pthread_t thread_id;
struct ThreadArgs ta;

struct State state;

// store all modules here
struct Module *m;

static gboolean on_switch_stackpage_cb(GtkWidget *widget, GVariant *args, gpointer data)
{
    gtk_stack_set_visible_child_name(GTK_STACK(widget), (char*)data);
    return 1;
}

gint on_glist_find_custom_cb(gconstpointer a, gconstpointer b)
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

int on_accept_cb(void* arg)
{
    gtk_widget_set_visible(GTK_WIDGET(arg), 1);
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

gboolean on_get_focus_cb(GtkWidget *widget, gpointer data)
{
    struct Module *m = data;
    module_unlock(m);
    return 1;
}

gboolean on_lost_focus_cb(GtkWidget *widget, gpointer data)
{
    struct Module *m = data;
    module_lock(m);
    return 1;
}

gboolean on_stack_moved_focus_cb(GtkWidget *widget, GObject *pspec, gpointer data)
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

static void app_activate_cb(GtkApplication *app)
{
    //GtkBuilder *builder = gtk_builder_new_from_file("src/gui/gui.ui");
    GtkBuilder *builder = gtk_builder_new_from_resource("/resources/ui/gui.ui");
    GObject *win = gtk_builder_get_object(builder, "main_win");
    GObject *w_stack = gtk_builder_get_object(builder, "main_stack");

    gtk_window_set_application(GTK_WINDOW(win), app);

    if (state.hide_on_close) {
        gtk_window_set_hide_on_close(GTK_WINDOW(win), TRUE);

        // Thread listens on unix domain socket for connections
        // If a connection is made it will show the window
        // When window is closed GTK will hide it instead of closing
        //ta = malloc(sizeof(struct ThreadArgs));
        ta.cb = on_accept_cb;
        ta.arg = (void*)win;
        ta.stop = 0;
        printf("Starting listener thread: 0X%lX\n", thread_id);
        pthread_create(&thread_id, NULL, listen_for_conn, (void*)&ta);
    }


    m = module_init(NULL, "apps",          apps_gui_init, NULL);
    m = module_init(m,    "notifications", notifications_gui_init, NULL);
    m = module_init(m,    "search",        search_gui_init, NULL);
    m = module_init(m,    "mpris",         mpris_gui_init, NULL);
//    m = module_init(m,    "dashboard",     dashboard_gui_init, NULL);

    struct Module *tmp = m->head;

    while (tmp != NULL) {
        module_activate(tmp);

        GtkStackPage *page = gtk_stack_add_child(GTK_STACK(w_stack), GTK_WIDGET(tmp->widget));
        gtk_stack_page_set_title(page, tmp->name);
        gtk_stack_page_set_name(page, tmp->name);
        tmp = tmp->next;
    }
    
    // lock all modules except for visible one (lock threads/running processes)
    on_stack_moved_focus_cb(GTK_WIDGET(w_stack), NULL, m->head);
    g_signal_connect(GTK_STACK(w_stack), "notify::visible-child", G_CALLBACK(on_stack_moved_focus_cb), m->head);

    module_debug(m);

    //gtk_stack_set_visible_child_name(GTK_STACK(w_stack), state.focus_page);
    
    GtkRecentManager *manager = gtk_recent_manager_get_default ();
    

    setup_keys(GTK_WIDGET(w_stack));

    gtk_window_present(GTK_WINDOW(win));
    g_object_unref(builder);
}

void show_help()
{
    printf("HUD :: Hud stuff\n");
    printf("Optional args:\n");
    printf("  -f <name>     Focus page at start\n");
    printf("  -H            Hide window instead of close\n");
    printf("  -D            Debug\n");
    printf("  -h            Show help\n");
}

int parse_args(struct State *s, int argc, char **argv)
{
    int option;
    printf("Parsing args\n");

    while((option = getopt(argc, argv, "f:HhD")) != -1) {
        switch (option) {
            case 'f':
                strcpy(s->focus_page, optarg);
                break;
            case 'H':
                s->hide_on_close = 1;
                break;
            case 'D': 
                s->do_debug = 1;
                break;
            case ':': 
                fprintf(stderr, "Option needs a value\n"); 
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

void init_state(struct State *s)
{
    /* Set defaults, can be overridden in parse_args() */
    s->hide_on_close = 0;
    s->do_debug = 1;

    strcpy(s->focus_page, "apps");
}

int main(int argc, char **argv)
{
    init_state(&state);
    if (parse_args(&state, argc, argv) < 0)
        return 1;

    GtkApplication *app = gtk_application_new("com.eco.hud", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate_cb), NULL);

    int stat = g_application_run(G_APPLICATION(app), 0, NULL);
    //int stat = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    if (state.hide_on_close == 1) {
        ta.stop = 1;
        pthread_join(thread_id, NULL);
    }
    return stat;
}
