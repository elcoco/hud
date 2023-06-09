#include "rg.h"


struct RGLine* rgline_init(struct RGLine *prev)
{
    struct RGLine *l = malloc(sizeof(struct RGLine));
    if (prev != NULL) {
        prev->next = l;
    }
    l->next = NULL;

    return l;
}

void rgline_print(struct RGLine *l)
{
    printf(">> %s\n", l->path);
    printf("   %s\n", l->text);
}

int rg_run(char *search, struct RGLine *l)
{
    /* Run Ripgrep and return hits as linked list by argument.
     * Function returns amount of hits or error if <0
     */
    int nfound = 0;
    struct RGLine *tmp = l;

    char cmd[RG_MAXBUF] = "";
    sprintf(cmd, "%s %s %s %s 2> /dev/null", RIPGREP_BIN_PATH, 
                                             RIPGREP_ARGS, 
                                             search,
                                             RIPGREP_PWD);

    FILE *pipe = popen(cmd, "r");

    printf("exec: %s\n", cmd);
    while (!feof(pipe)) {
        char buf[RG_MAXBUF] = "";

        // check end of data
        if (fgets(buf, sizeof(buf), pipe) == NULL)
            break;
        
        buf[strlen(buf)-1] = '\0';
        JSONObject* rn = json_load(buf);

        json_print(rn, 0);

        if (rn == NULL) {
            fprintf(stderr, "Error in json\n");
            goto on_err;
        }

        // filter by only checking match type
        struct JSONObject *ntype = json_get_path(rn, "type");

        if (ntype == NULL)
            continue;

        if (strcmp(ntype->value, "match") != 0)
            continue;

        struct JSONObject *npath = json_get_path(rn, "data/path/text");
        struct JSONObject *ntext = json_get_path(rn, "data/lines/text");

        if (npath == NULL || ntext == NULL)
            continue;

        // connect next in linked list
        if (nfound != 0)
            tmp = rgline_init(tmp);

        tmp->path = strdup(npath->value);
        tmp->text = strdup(ntext->value);

        nfound++;

        json_obj_destroy(rn);
    }
    pclose(pipe);
    return nfound;

on_err:
    pclose(pipe);
    return -1;
}

void rg_request(char *search)
{
    struct RGLine *l = rgline_init(NULL);

    if (rg_run(search, l) > 0) {

        struct RGLine *tmp = l;

        while (tmp != NULL) {
            rgline_print(tmp);
            tmp = tmp->next;
        }
    }
}
