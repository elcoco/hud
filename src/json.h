#ifndef JSON_H
#define JSON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>     // for utf8 in curses
#include <stdarg.h>
                        //
#define MAX_BUF 1024 * 10
#define LINES_CONTEXT 100
#define PATH_DELIM "/"

#define JRESET   "\x1B[0m"
#define JRED     "\x1B[31m"
#define JGREEN   "\x1B[32m"
#define JYELLOW  "\x1B[33m"
#define JBLUE    "\x1B[34m"
#define JMAGENTA "\x1B[35m"
#define JCYAN    "\x1B[36m"
#define JWHITE   "\x1B[37m"

#define JCOL_NUM       JGREEN
#define JCOL_STR       JBLUE
#define JCOL_BOOL      JMAGENTA
#define JCOL_OBJ       JGREEN
#define JCOL_ARR       JGREEN
#define JCOL_KEY       JWHITE
#define JCOL_ARR_INDEX JRED
#define JCOL_UNKNOWN   JRED

enum JSONDtype {
    JSON_STRING,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_BOOL,
    JSON_ARRAY,
    JSON_UNKNOWN
};

// End of file should not be reached
enum JSONStatus {
    END_OF_FILE    = -3,
    PARSE_ERROR    = -2,
    STATUS_ERROR   = -1,
    STATUS_SUCCESS = 0,
    END_OF_ARRAY   = 1,
    END_OF_OBJECT  = 2
};

struct JSONObject {

    /* When object is the root object, parent is NULL */
    struct JSONObject* parent;

    enum JSONDtype dtype;

    // When object is child of array, the key is NULL
    char* key;

    /* In case of int/float/bool/string:
     *      - value can be accessed by using one of the helper functions
     */
    void* value;

    /* In case of array/object:
     *      - array length is accessed by using jo->length attribute
     *      - children can be iterated as a linked list by using value as the head node */

    // in case of object or array, this stores the linked list length
    int32_t length;

    // index to the array or object
    int32_t index;

    // If jsonobject is part of an array this gives access to its siblings
    struct JSONObject* next;
    struct JSONObject* prev;

    bool is_bool;
    bool is_string;
    bool is_number;
    bool is_array;
    bool is_object;
};

/* When parsing a json string, this struct holds a pointer to the current position in the string */
struct Position {
    uint32_t npos;      // char counter
    char *json;         // full json string
    char *c;            // pointer to current char in json string
    uint64_t length;    // json string length

    // row/col counter used to report position of json errors
    uint32_t rows;      
    uint32_t cols;
};

struct JSONObject* json_load(char* buf);
struct JSONObject* json_load_file(char *path);
void json_print(struct JSONObject* jo, uint32_t level);
void json_obj_destroy(struct JSONObject* jo);
char* json_object_to_string(struct JSONObject *jo, int spaces);
int json_object_to_file(struct JSONObject *jo, char *path, int spaces);

struct JSONObject* json_object_init(struct JSONObject* parent);

// get value casted to the appropriate type
char* json_get_string(struct JSONObject* jo);
double json_get_number(struct JSONObject* jo);
bool json_get_bool(struct JSONObject* jo);

struct JSONObject *json_get_path(struct JSONObject *rn, char *buf);
struct JSONObject *json_set_path(struct JSONObject *rn, char *buf, struct JSONObject *child);

#endif
