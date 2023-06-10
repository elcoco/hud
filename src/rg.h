#ifndef RG_H

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

#define RG_MAXBUF 1024 * 10

struct RGLine;

struct RGLine {
    char *text;
    char *path;

    struct RGLine *next;
};

void rgline_print_all(struct RGLine *l);
void rg_test(char *search);
int rg_request(char *search, struct RGLine *l);

#endif // !RG_H
