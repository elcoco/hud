#ifndef APPS_H
#define APPS_H

#include <ctype.h>      // to_lower()
                        //
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "module.h"


#define APP_TYPE_ITEM (app_item_get_type ())
G_DECLARE_FINAL_TYPE(AppItem, app_item, APP, ITEM, GObject)
struct _AppItem {
    GObject parent_instance;

    GAppInfo *app_info;
};

#define APPS_UI_PATH "/resources/ui/gui.ui"

GObject* apps_gui_init(struct Module *m);

#endif // !APPS_H
