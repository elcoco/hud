#include "config.h"
#include "json.h"

struct Config* config_init(char *path)
{
    struct Config* c = malloc(sizeof(struct Config));
    c->path = strdup(path);


    return c;
}

enum ConfigReturn config_file_exists(struct Config *c)
{
    FILE *fp = fopen(c->path, "r");
    if (fp == NULL) {
        printf("CONFIG: File doesn't exist: %s\n", c->path);
        return CONFIG_FILE_NOT_FOUND;
    }

    fclose(fp);
    return 1;
}

enum ConfigReturn config_file_create(struct Config *c)
{
    struct JSONObject* rn = json_object_init_object(NULL, NULL);
    if (rn == NULL)
        return CONFIG_PARSE_ERROR;

    if (json_object_to_file(rn, c->path, 4) < 0)
        return CONFIG_FILE_ERROR;

    return CONFIG_SUCCESS;
}


void config_destroy(struct Config *c)
{
    free(c->path);
    free(c);
}

enum ConfigReturn config_set_str(struct Config *c, char *path, char *key, char *value)
{
    struct JSONObject *jo = json_load_file(c->path);

    if (jo == NULL)
        return CONFIG_PARSE_ERROR;

    struct JSONObject *child = json_object_init_string(NULL, key, value);
    
    json_set_path(jo, path, child);
    json_object_to_file(jo, c->path, 4);

    return CONFIG_SUCCESS;
}

enum ConfigReturn config_get_int(struct Config *c, char *path, int *value)
{
    struct JSONObject *jo = json_load_file(c->path);
    struct JSONObject *item = json_get_path(jo, path);
    if (item == NULL)
        return CONFIG_ERROR;

    *value = json_get_number(item);
    return CONFIG_SUCCESS;
}
