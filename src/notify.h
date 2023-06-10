#ifndef NOTIFICATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "json.h"

#define NOTIFY_AMOUNT 20
#define NOTIFY_BIN_PATH "/usr/bin/dunstctl"
#define NOTIFY_ARGS "history"
#define NOTIFY_MAXBUF 1024 * 10

struct NotifyItem {
    char *app;
    char *summary;
    char *body;
    struct NotifyItem *next;
};

int notify_req(int amount, struct NotifyItem *ni);
void notify_test(int amount);

#endif // !NOTIFICATIONS_H
