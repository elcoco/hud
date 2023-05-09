#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "json.h"


#define RIPGREP_BIN_PATH "/usr/bin/rg"
#define RIPGREP_ARGS     "--json"
//#define RIPGREP_ARGS     "-l --color never"
#define RIPGREP_PWD      "~"
#define RIPGREP_SEARCH   "next"

#define MAXBUF 512


void die(char *fmt, ...)
{
    va_list ptr;
    va_start(ptr, fmt);
    vfprintf(stderr, fmt, ptr);
    va_end(ptr);
    exit(1);
}

int main()
{
    char cmd[MAXBUF] = "";
    sprintf(cmd, "%s %s %s %s", RIPGREP_BIN_PATH, 
                                RIPGREP_ARGS, 
                                RIPGREP_SEARCH, 
                                RIPGREP_PWD);

    FILE *pipe = popen(cmd, "r");

    while (!feof(pipe)) {
        char buf[MAXBUF] = "";
        if (fgets(buf, sizeof(buf), pipe) != NULL) {
            buf[strlen(buf)-1] = '\0';
            printf(">>%s\n", buf);

            JSONObject* rn = json_load(buf);
            if (rn == NULL)
                die("error in json\n");
        }
    }
    pclose(pipe);
    return 0;
}
