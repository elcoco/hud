#include "utils.h"

void str_to_lower(char *buf)
{
    for (int i=0 ; i<strlen(buf) ; i++)
        buf[i] = tolower(buf[i]);
}

int find_substr(const char *haystack, const char *needle)
{
    char *str = strdup(haystack);
    char *substr = strdup(needle);
    str_to_lower(str);
    str_to_lower(substr);

    int res = (strstr(str, substr)) ? 1 : 0;

    free(str);
    free(substr);

    return res;
}

size_t str_alloc(char **buf, size_t old_size, size_t wanted_size, size_t chunk_size)
{
    /* Grow string one chunk_size at a time if old_size < wanted_size
     * If *buf == NULL, do an initial initialization
     */
    size_t new_size = ((wanted_size / chunk_size) +1) * chunk_size;

    // on first call with uninitialized char do a malloc
    if (*buf == NULL) {
        *buf = malloc(new_size);
        *buf[0] = '\0';
        printf("init -> %ld\n", new_size);
        return new_size;
    }

    if (old_size < wanted_size) {
        printf("grow %ld -> %ld\n", old_size, new_size);
        *buf = realloc(*buf, new_size);
        return new_size;
    }

    return old_size;
}

int get_line_from_pipe(FILE* pipe, char **buf)
{
    size_t chunk_size = 256;
    size_t buf_size = 0;

    while (!feof(pipe)) {

        if (*buf == NULL) {
            buf_size = chunk_size;
            *buf = malloc(buf_size+1);
            (*buf)[0] = '\0';
            //printf("from pipe init -> %ld\n", buf_size);
        }

        // reads till chunk_size or '\n'
        fgets((*buf)+strlen(*buf), chunk_size, pipe);

        if ((*buf)[strlen(*buf)-1] == '\n') {
            //printf("end of line\n");
            break;
        }
        else {
            buf_size += chunk_size;
            *buf = realloc(*buf, buf_size+1);
            //printf("from pipe grow -> %ld\n", buf_size);
        }

    }
    //printf("read: >%s<\n", *buf);

    return buf_size;
}

int get_all_from_pipe(FILE* pipe, char **buf)
{
    size_t chunk_size = 256;
    int nread = 0;

    size_t buf_size = chunk_size;
    *buf = malloc(buf_size+1);

    while (!feof(pipe)) {
        nread += fread((*buf)+nread, 1, chunk_size, pipe);

        buf_size += chunk_size;
        *buf = realloc(*buf, buf_size+1);
    }
    // end string and cut off newline
    if (nread > 0)
        (*buf)[nread-1] = '\0';
    else
        (*buf)[0] = '\0';

    fclose(pipe);
    return nread;
}

int ts_to_time_elapsed(time_t t, char *buf)
{
    time_t t_diff = time(NULL) - t;
    if (t_diff >= 60 * 60 * 24)
        sprintf(buf, "%ld days ago", t_diff/(60*60*24)); 
    else if (t_diff >= 60 * 60)
        sprintf(buf, "%ld hours ago", t_diff/(60*60)); 
    else if (t_diff >= 60)
        sprintf(buf, "%ld minutes ago", t_diff/60); 
    else
        sprintf(buf, "%ld seconds ago", t_diff); 
    return 0;
}


GAppInfo* find_appinfo(const char *app_name)
{
    // lookup icon
    GList *apps = g_app_info_get_all();
    GList *app = apps;
    GAppInfo *match = NULL;

    while (app != NULL) {
        GAppInfo *app_info = app->data;
        const char *name = g_app_info_get_name(app_info);

        if (strcmp(name, app_name) == 0) {
            match = g_app_info_dup(app_info);
            break;
        }
        g_object_unref(app_info);
        app = app->next;
    }
    g_list_free(apps);
    return match;
}
