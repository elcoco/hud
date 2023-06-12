#include "notify.h"
#include "json.h"

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
        printf("growing buf (%ld): %ld -> %ld\n", strlen(buf), size, size + grow_size);
        size += grow_size;
        return realloc(buf, size);
    }
    else {
        printf("nothing to do\n");
        return buf;
    }
}

int notify_req(int amount, struct NotifyItem *ni)
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

    while (!feof(pipe)) {

        buf = notify_buf_grow(buf, NOTIFY_BUF_GROW_SIZE);

        // read line by line
        if (fgets(buf+strlen(buf), NOTIFY_BUF_GROW_SIZE, pipe) == NULL) {
            break;
        }
        
        //printf(">>%s<<\n", buf);
    }
    buf[strlen(buf)-1] = '\0';
    printf(">>%s<<\n", buf);
    JSONObject* rn = json_load(buf);

    if (rn == NULL || !rn->is_object) {
        fprintf(stderr, "Failed to get JSON from dunst");
        return -1;
    }

    // TODO path with mixed keys/array indices should be possible

    printf("FOUND!!!!\n\n");
    struct JSONObject *n1 = json_get_path(rn, "data");
    struct JSONObject *msgs = n1->children[0];
    struct JSONObject *msg = msgs->value;


    struct NotifyItem *ni_tmp = ni;

    while (msg != NULL) {
        json_print(msg, 0);

        struct JSONObject *nbody = json_get_path(msg, "body/data");
        struct JSONObject *nmsg  = json_get_path(msg, "message/data");
        struct JSONObject *nsum = json_get_path(msg, "summary/data");
        struct JSONObject *napp  = json_get_path(msg, "app/data");
        struct JSONObject *nts   = json_get_path(msg, "timestamp/data");

        ni_tmp->body    = strdup(nbody->value);
        ni_tmp->msg     = strdup(nmsg->value);
        ni_tmp->summary = strdup(nsum->value);
        ni_tmp->app     = strdup(napp->value);
        ni_tmp->ts      = json_get_number(nts);

        ni_tmp = notify_init(ni_tmp);
        msg = msg->next;
    }

    
    return 0;

}

void notify_test(int amount)
{
    struct NotifyItem *ni = notify_init(NULL);

    if (notify_req(amount, ni) > 0)
        notify_print_all(ni);
}
