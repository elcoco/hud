#ifndef SEARCH_GUI_H
#define SEARCH_GUI_H

#include <ctype.h>      // to_lower()
#include <time.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "rg.h"
#include "utils.h"
#include "job.h"
#include "module.h"

#define RG_AMOUNT_RESULTS 1000
#define SEARCH_UI_PATH "/resources/ui/gui.ui"
#define SEARCH_ITEM_UI_PATH "/resources/ui/gui.ui"
#define SEARCH_ENTRY_DELAY 500

#define SEARCH_TYPE_ITEM (search_item_get_type ())
G_DECLARE_FINAL_TYPE(SearchItem, search_item, SEARCH, ITEM, GObject)
struct _SearchItem {
    GObject parent_instance;
    const char *path;
    const char *text;
    int lineno;
};

struct SearchResult {
    GListStore *model;
    GtkListView *view;
    GtkLabel *label;
    GtkSelectionModel *sel_model;
    int nfound;
    struct RGLine *result;
    char search_text[256];
};


GObject* search_gui_init(struct Module*);


#endif // !GUI_H
