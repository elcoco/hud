#include "notify.h"

struct NotifyItem* notify_init(struct NotifyItem *prev)
{
    struct NotifyItem *ni = malloc(sizeof(struct NotifyItem));
    if (prev != NULL) {
        prev->next = ni;
    }
    ni->next = NULL;

    return ni;
}

void notify_print(struct NotifyItem *ni)
{
    printf(">> %s: %s %s\n", ni->app, ni->summary, ni->body);
}

void notify_print_all(struct NotifyItem *ni)
{
}

int notify_req(int amount, struct NotifyItem *ni)
{
    /* Get amount of notifications from dunstctl
     * If amount == -1, return all
     */
    // TODO: get data from dunstctl and increase buffer size till all data is in

    struct NotifyItem *tmp = ni;

    char cmd[NOTIFY_MAXBUF] = "";
    sprintf(cmd, "%s %s 2> /dev/null", NOTIFY_BIN_PATH, NOTIFY_ARGS);

    FILE *pipe = popen(cmd, "r");

    printf("exec: %s\n", cmd);
    while (!feof(pipe)) {
        char buf[NOTIFY_MAXBUF] = "";

        // check end of data
        if (fgets(buf, sizeof(buf), pipe) == NULL)
            break;
        
        buf[strlen(buf)-1] = '\0';
        JSONObject* rn = json_load(buf);

        json_print(rn, 0);
    }

    return 0;

}

void notify_test(int amount)
{
    struct NotifyItem *ni = notify_init(NULL);

    if (notify_req(amount, ni) > 0)
        notify_print_all(ni);
}
