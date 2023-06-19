#ifndef RG_H
#define RG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "json.h"
#include "utils.h"


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
    int lineno;

    struct RGLine *next;
};

void rgline_print_all(struct RGLine *l);
void rg_test(char *search);
//int rg_request(char *search, struct RGLine *l);
//int rg_request(const char *search, struct RGLine *l, int amount);
int rg_request(const char *search, struct RGLine *l, int amount, int *do_stop);
struct RGLine* rgline_init(struct RGLine *prev);
void rgline_destroy(struct RGLine *l);

#endif // !RG_H
