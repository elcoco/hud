#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "json.h"
#include "utils.h"

#define NOTIFY_AMOUNT 20
#define NOTIFY_BIN_PATH "/usr/bin/dunstctl"
#define NOTIFY_ARGS "history"
#define NOTIFY_MAXBUF 1024 * 10
#define NOTIFY_BUF_GROW_SIZE 256

struct NotifyItem {
    char *app;
    char *summary;
    char *body;
    char *msg;
    time_t ts;
    struct NotifyItem *next;
};

int notify_req(int amount, struct NotifyItem *ni);
void notify_test(int amount);
struct NotifyItem* notify_init(struct NotifyItem *prev);

#endif // !NOTIFICATIONS_H
