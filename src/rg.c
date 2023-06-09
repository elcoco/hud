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
        if (fgets(buf, sizeof(buf), pipe) == NULL) {
            fprintf(stderr, "End of data\n");
            return nfound;
        }
        
        buf[strlen(buf)-1] = '\0';

        JSONObject* rn = json_load(buf);

        if (rn == NULL) {
            fprintf(stderr, "Error in json\n");
            return -1;
        }

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
