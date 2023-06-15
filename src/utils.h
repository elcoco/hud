#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>      // to_lower()
                        
void str_to_lower(char *buf);
int find_substr(const char *haystack, const char *needle);

#endif // !UTILS_H
