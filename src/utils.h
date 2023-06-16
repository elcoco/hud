#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>      // to_lower()
                        //
                        
void str_to_lower(char *buf);
int find_substr(const char *haystack, const char *needle);
size_t str_alloc(char **buf, size_t old_size, size_t wanted_size, size_t chunk_size);
int get_line_from_pipe(FILE* pipe, char **buf);
int get_all_from_pipe(FILE* pipe, char **buf);

#endif // !UTILS_H
