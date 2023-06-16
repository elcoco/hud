#ifndef SEARCH_GUI_H
#define SEARCH_GUI_H

#include <ctype.h>      // to_lower()
#include <time.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "rg.h"
#include "utils.h"

#define RG_AMOUNT_RESULTS 1000
#define SEARCH_UI_PATH "/resources/ui/gui.ui"
#define SEARCH_ITEM_UI_PATH "/resources/ui/gui.ui"

#define SEARCH_TYPE_ITEM (search_item_get_type ())
G_DECLARE_FINAL_TYPE(SearchItem, search_item, SEARCH, ITEM, GObject)
struct _SearchItem {
    GObject parent_instance;
    const char *path;
    const char *text;
    int lineno;
};


GObject* search_gui_init();


#endif // !GUI_H
