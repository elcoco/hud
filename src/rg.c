#include "rg.h"


struct RGLine* rgline_init(struct RGLine *prev)
{
    struct RGLine *l = malloc(sizeof(struct RGLine));
    if (prev != NULL)
        prev->next = l;
    l->next = NULL;

    return l;
}

void rgline_print(struct RGLine *l)
{
    printf(">> %s\n", l->path);
    printf("   %s\n", l->text);
}

void rgline_print_all(struct RGLine *l)
{
    struct RGLine *tmp = l;

    while (tmp != NULL) {
        rgline_print(tmp);
        tmp = tmp->next;
    }
}

int rg_request(const char *search, struct RGLine *l, int amount)
{
    /* Run Ripgrep and return hits as linked list by argument.
     * Function returns amount of hits or error if <0
     */
    int nfound = 0;
    struct RGLine *tmp = l;

    char cmd[RG_MAXBUF] = "";
    sprintf(cmd, "%s %s \"%s\" %s 2> /dev/null", RIPGREP_BIN_PATH, 
                                             RIPGREP_ARGS, 
                                             search,
                                             RIPGREP_PWD);

    FILE *pipe = popen(cmd, "r");
    printf("exec: %s\n", cmd);

    while (!feof(pipe)) {
        if (nfound == amount)
            break;

        char *buf = NULL;
        get_line_from_pipe(pipe, &buf);

        // nothing read, eof
        if (buf == NULL)
            break;
        
        // delete '\n'
        buf[strlen(buf)-1] = '\0';

        JSONObject* rn = json_load(buf);
        free(buf);

        if (rn == NULL) {
            fprintf(stderr, "Error in json\n");
            goto on_err;
        }

        // filter by only checking match type results
        struct JSONObject *ntype = json_get_path(rn, "type");

        if (ntype == NULL || strcmp(ntype->value, "match") != 0) {
            json_obj_destroy(rn);
            continue;
        }

        struct JSONObject *npath  = json_get_path(rn, "data/path/text");
        struct JSONObject *ntext  = json_get_path(rn, "data/lines/text");
        struct JSONObject *lineno = json_get_path(rn, "data/line_number");

        if (npath == NULL || ntext == NULL || lineno == NULL) {
            json_obj_destroy(rn);
            continue;
        }

        //printf(">>%s %s %f<<\n", (char*)(npath->value), (char*)(ntext->value), json_get_number(lineno));
        
        // connect next in linked list
        if (nfound != 0)
            tmp = rgline_init(tmp);

        assert(npath->value != NULL);
        assert(ntext->value != NULL);
        assert(lineno->value != NULL);


        tmp->path = strdup(npath->value);
        tmp->text = strdup(ntext->value);
        tmp->lineno = json_get_number(lineno);

        nfound++;

        json_obj_destroy(rn);
    }
    pclose(pipe);
    return nfound;

on_err:
    pclose(pipe);
    return -1;
}

void rg_test(char *search)
{
    struct RGLine *l = rgline_init(NULL);

    if (rg_request(search, l, 100) > 0)
        rgline_print_all(l);
}
