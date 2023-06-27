#ifndef MPRIS_GUI_H
#define MPRIS_GUI_H

#include <stdint.h>

#include "module.h"
#include "mpris.h"
#include "utils.h"


#define MPRIS_TYPE_ITEM (mpris_item_get_type ())
G_DECLARE_FINAL_TYPE(MPRISItem, mpris_item, MPRIS, ITEM, GObject)
struct _MPRISItem {
    GObject parent_instance;
    struct MPRISPlayer *mp;

    char *namespace;
    int first_run;
    

};

#define MPRIS_UI_PATH "/resources/ui/gui.ui"
#define MPRIS_UPDATE_INTERVAL_MS 1000



GObject* mpris_gui_init(struct Module *m);

#endif // !MPRIS_GUI_H
