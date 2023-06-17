#include "notifications.h"

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

char* notify_buf_grow(char *buf, size_t grow_size)
{
    /* Grow char array if necessary.
     * If buf == NULL, do an initial initialization
     */
    static size_t size = 0;

    // on first call with uninitialized char array reset static var
    if (buf == NULL) {
        size = grow_size;
        buf = malloc(size);
        buf[0] = '\0';
        printf("init\n");
        return buf;
    }
    else if (strlen(buf) + grow_size > size) {
        size += grow_size;
        return realloc(buf, size);
    }
    else {
        return buf;
    }
}

time_t get_uptime()
{
    char uptime_chr[28];
    long uptime = 0;

    FILE* uptimefile = fopen("/proc/uptime", "r");
    fgets(uptime_chr, 12, uptimefile);
    fclose(uptimefile);
    uptime = strtol(uptime_chr, NULL, 10);

    //printf("System up for %ld seconds, %ld hours\n", uptime, uptime / 3600);
    return uptime;
}

struct NotifyItem* notify_req(int amount)
{
    /* Get amount of notifications from dunstctl
     * If amount == -1, return all
     */
    // TODO: get data from dunstctl and increase buffer size till all data is in

    char cmd[NOTIFY_MAXBUF] = "";
    sprintf(cmd, "%s %s 2> /dev/null", NOTIFY_BIN_PATH, NOTIFY_ARGS);

    FILE *pipe = popen(cmd, "r");

    printf("exec: %s\n", cmd);

    char *buf = NULL;
    get_all_from_pipe(pipe, &buf);
        
    JSONObject* rn = json_load(buf);

    if (rn == NULL || !rn->is_object) {
        fprintf(stderr, "Failed to get JSON from dunst");
        return NULL;
    }

    // TODO path with mixed keys/array indices should be possible
    struct JSONObject *n1 = json_get_path(rn, "data");

    assert(n1 != NULL);

    struct JSONObject *msgs = n1->children[0];
    struct JSONObject *msg = msgs->value;

    struct NotifyItem *ni_tmp = NULL;
    struct NotifyItem *ni_head = NULL;

    while (msg != NULL) {
        ni_tmp = notify_init(ni_tmp);

        if (ni_head == NULL)
            ni_head = ni_tmp;

        struct JSONObject *nbody = json_get_path(msg, "body/data");
        struct JSONObject *nmsg  = json_get_path(msg, "message/data");
        struct JSONObject *nsum  = json_get_path(msg, "summary/data");
        struct JSONObject *napp  = json_get_path(msg, "app/data");
        struct JSONObject *nts   = json_get_path(msg, "timestamp/data");

        
        ni_tmp->body    = strdup(nbody->value);
        ni_tmp->msg     = strdup(nmsg->value);
        ni_tmp->summary = strdup(nsum->value);
        ni_tmp->app     = strdup(napp->value);

        // dunst timestamp is microseconds (?) since boot time
        ni_tmp->ts = time(NULL) - (get_uptime() - (time_t)json_get_number(nts)/1000000);

        msg = msg->next;
    }
    return ni_head;
}
