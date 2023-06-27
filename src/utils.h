#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>      // to_lower()
#include <time.h>

#include <gtk/gtk.h>


// custom assert macro with formatted message and ui cleanup
#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {ui_cleanup() ; log_error(M, ##__VA_ARGS__); assert(A); }
                        
void str_to_lower(char *buf);
int find_substr(const char *haystack, const char *needle);
size_t str_alloc(char **buf, size_t old_size, size_t wanted_size, size_t chunk_size);
int get_line_from_pipe(FILE* pipe, char **buf);
int get_all_from_pipe(FILE* pipe, char **buf);
int ts_to_time_elapsed(time_t t, char *buf);
GAppInfo* find_appinfo(const char *app_name);

void update_prop_str(GObject *item, const char *key, const char *value, int force);
void update_prop_uint64(GObject *item, const char *key, uint64_t value, int force);
void update_prop_int(GObject *item, const char *key, int value, int force);

#endif // !UTILS_H
