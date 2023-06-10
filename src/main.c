#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "json.h"
#include "notify.h"
//#include "gui.h"


#define RIPGREP_BIN_PATH "/usr/bin/rg"
#define RIPGREP_ARGS     "--json"
//#define RIPGREP_ARGS     "-l --color never"
#define RIPGREP_PWD      "~"
#define RIPGREP_SEARCH   "next"

#define MAXBUF 1024 * 10



void die(char *fmt, ...)
{
    va_list ptr;
    va_start(ptr, fmt);
    vfprintf(stderr, fmt, ptr);
    va_end(ptr);
    exit(1);
}


int main(int argc, char **argv)
{
    //start_gui(argc, argv);
    //rg_test("banaan");
    notify_test(5);

    return 0;
}
