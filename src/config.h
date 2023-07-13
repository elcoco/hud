#ifndef CONFIG_H
#define CONFIG_H

#include "json.h"
#include "utils.h"

struct Config {
    char *path;
};

enum ConfigReturn {
    CONFIG_FILE_ERROR     = -4,
    CONFIG_PARSE_ERROR    = -3,
    CONFIG_FILE_NOT_FOUND = -2,
    CONFIG_ERROR          = -1,
    CONFIG_SUCCESS        = 0,
    CONFIG_FILE_CREATED   = 1,
};

struct Config* config_init(char *path);
void config_destroy(struct Config *c);
enum ConfigReturn config_file_exists(struct Config *c);
enum ConfigReturn config_set_str(struct Config *c, const char *path, const char *key, const char *value);
enum ConfigReturn config_set_number(struct Config *c, const char *path, const char *key, int value);
enum ConfigReturn config_file_create(struct Config *c);

enum ConfigReturn config_get_str(struct Config *c, char *path, char **buf);
enum ConfigReturn config_get_int(struct Config *c, char *path, int *value);
enum ConfigReturn config_get_child(struct Config *c, char *path, struct JSONObject **jo);

enum ConfigReturn config_array_insert(struct Config *c, char *path, struct JSONObject *child, int index);

#endif // !CONFIG_H
