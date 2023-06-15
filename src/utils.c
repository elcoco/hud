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
