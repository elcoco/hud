#ifndef DOCK_H
#define DOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "module.h"
#include "app_model.h"
#include "utils.h"

#define DOCK_UI_PATH "/resources/ui/gui.ui"
#define DOCK_CONFIG_PATH "modules/dock/apps"

GObject* dock_gui_init(struct Module *m);

#endif // !DOCK_H
