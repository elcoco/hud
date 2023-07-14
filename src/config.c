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
        ERROR("CONFIG: File doesn't exist: %s\n", c->path);
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

enum ConfigReturn config_set_str(struct Config *c, const char *path, const char *key, const char *value)
{
    struct JSONObject *jo = json_load_file(c->path);

    if (jo == NULL)
        return CONFIG_PARSE_ERROR;

    struct JSONObject *child = json_object_init_string(NULL, key, value);
    
    json_set_path(jo, path, child);
    json_object_to_file(jo, c->path, 4);
    json_print(jo, 0);

    return CONFIG_SUCCESS;
}

enum ConfigReturn config_set_number(struct Config *c, const char *path, const char *key, int value)
{
    struct JSONObject *jo = json_load_file(c->path);

    if (jo == NULL)
        return CONFIG_PARSE_ERROR;

    struct JSONObject *child = json_object_init_number(NULL, key, value);
    
    json_set_path(jo, path, child);
    json_object_to_file(jo, c->path, 4);
    json_print(jo, 0);

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

enum ConfigReturn config_get_str(struct Config *c, char *path, char **buf)
{
    struct JSONObject *jo = json_load_file(c->path);
    struct JSONObject *item = json_get_path(jo, path);
    if (item == NULL)
        return CONFIG_ERROR;

    *buf = json_get_string(item);
    return CONFIG_SUCCESS;
}

enum ConfigReturn config_get_child(struct Config *c, char *path, struct JSONObject **jo)
{
    struct JSONObject *rn = json_load_file(c->path);
    *jo = json_get_path(rn, path);

    if (*jo == NULL)
        return CONFIG_ERROR;

    return CONFIG_SUCCESS;
}

enum ConfigReturn config_remove_child(struct Config *c, char *path)
{
    struct JSONObject *jo = json_load_file(c->path);
    struct JSONObject *item = json_get_path(jo, path);
    if (item == NULL)
        return CONFIG_ERROR;

    json_object_destroy(item);
    json_object_to_file(jo, c->path, 4);
    return CONFIG_SUCCESS;
}

enum ConfigReturn config_array_insert(struct Config *c, char *path, struct JSONObject *child, int index)
{
    struct JSONObject *rn = json_load_file(c->path);

    if (rn == NULL)
        return CONFIG_ERROR;

    struct JSONObject *jo = json_get_path(rn, path);

    if (jo == NULL)
        return CONFIG_ERROR;

    if (!jo->is_array)
        return CONFIG_ERROR;


    int len = json_count_children(jo);
    if (index >= len) {
        json_object_append_child(jo, child);
    }
    else {
        if (!json_object_insert_child(jo, child, index))
            return CONFIG_ERROR;
    }

    json_object_to_file(rn, c->path, 4);
    return CONFIG_SUCCESS;
}



