#ifndef CONFIG_H
#define CONFIG_H

#include "json.h"

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
enum ConfigReturn config_set_str(struct Config *c, char *path, char *key, char *value);
enum ConfigReturn config_file_create(struct Config *c);

#endif // !CONFIG_H
