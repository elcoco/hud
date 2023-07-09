#include "json.h"

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

static enum JSONStatus json_parse(struct JSONObject* jo, struct Position* pos);
static enum JSONStatus json_parse_object(struct JSONObject* jo, struct Position* pos);
static enum JSONStatus json_parse_array(struct JSONObject* jo, struct Position* pos);
static enum JSONStatus json_parse_string(struct JSONObject* jo, struct Position* pos, char quote_chr);
static enum JSONStatus json_parse_key(struct JSONObject* jo, struct Position* pos);
static enum JSONStatus json_parse_bool(struct JSONObject* jo, struct Position* pos);
static enum JSONStatus json_parse_number(struct JSONObject* jo, struct Position* pos);
static void json_object_add_child(struct JSONObject *parent, struct JSONObject *child);
static char fforward_skip_escaped_grow(struct Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char** buf);
static int count_backslashes(struct Position *pos);
static char fforward_skip_escaped(struct Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char* buf);
static char* pos_next(struct Position *pos);
static void print_error(struct Position *pos, uint32_t amount);
static void get_spaces(char *buf, uint8_t spaces);
static int json_atou_err(char *str);
static int json_get_path_length(char *path);
static void debug(char* fmt, ...);
static int is_last_item(struct JSONObject *jo);
static void json_object_to_string_rec(struct JSONObject *jo, char **buf, uint32_t level, int spaces);
static void write_to_string(char **buf, char *fmt, ...);



static void debug(char* fmt, ...)
{
    char buf[MAX_BUF];

    va_list ptr;
    va_start(ptr, fmt);
    vsprintf(buf, fmt, ptr);
    va_end(ptr);
    printf("%s", buf);

    //FILE* fp = fopen(LOG_PATH, "a");
    //fputs(buf, fp);
    //fclose(fp);
}

static int is_last_item(struct JSONObject *jo)
{
    /* Check if object is last in an array or object to see
     * if we need to use a comma in json render */

    // is last in array/object
    if (jo->parent != NULL && (jo->parent->is_array || jo->parent->is_object))
        return jo->next == NULL;

    // is rootnode
    if (jo->parent == NULL && jo->length >= 0)
        return 1;
    return 0;
}

static int json_get_path_length(char *path)
{
    char tmp[256] = "";
    strncpy(tmp, path, strlen(path));
    strtok(tmp, PATH_DELIM);
    int i = 1;

    while(strtok(NULL, PATH_DELIM))
        i++;

    return i;
}

static void write_to_string(char **buf, char *fmt, ...)
{
    /* Allocate and write to string, return new size of allocated space */
    int chunk_size = 256;
    char src_buf[512] = "";

    va_list ptr;
    va_start(ptr, fmt);
    vsprintf(src_buf, fmt, ptr);
    va_end(ptr);

    if (*buf == NULL) {
        *buf = malloc(chunk_size);
        strcpy(*buf, "");
    }

    size_t old_nchunks = (strlen(*buf) / chunk_size) + 1;
    size_t new_nchunks = ((strlen(*buf) + strlen(src_buf)) / chunk_size) + 1;

    if (old_nchunks < new_nchunks)
        *buf = realloc(*buf, new_nchunks * chunk_size);

    strncat(*buf, src_buf, strlen(src_buf));
}

static void json_object_to_string_rec(struct JSONObject *jo, char **buf, uint32_t level, int spaces)
{
    /* The function that does the recursive string stuff */
    char space[level+1];
    get_spaces(space, level);

    if (jo != NULL) {
        write_to_string(buf, "%s", space);

        if (jo->key)
            write_to_string(buf, "\"%s\": ", jo->key);

        switch (jo->dtype) {

            case JSON_NUMBER:
                write_to_string(buf, "%f", json_get_number(jo));
                break;

            case JSON_STRING:
                write_to_string(buf, "\"%s\"", json_get_string(jo));
                break;

            case JSON_BOOL:
                write_to_string(buf, "%s", json_get_bool(jo) ? "true" : "false");
                break;

            case JSON_ARRAY:
                write_to_string(buf, "[\n");
                json_object_to_string_rec(jo->value, buf, level+spaces, spaces);
                write_to_string(buf, "%s]", space);

                break;

            case JSON_OBJECT:
                write_to_string(buf, "{\n");
                json_object_to_string_rec(jo->value, buf, level+spaces, spaces);
                write_to_string(buf, "%s}", space);
                break;

            case JSON_UNKNOWN:
                debug("%s[UNKNOWN]%s\n", buf, JCOL_UNKNOWN, JRESET);
                break;
        }
        if (!is_last_item(jo))
            write_to_string(buf, ",\n");
        else
            write_to_string(buf, "\n");

        if (jo->next != NULL)
            json_object_to_string_rec(jo->next, buf, level, spaces);
    }
}

static int json_atou_err(char *str)
{
    int num;
    char *endptr;

    num = strtol(str, &endptr, 0);
    if (endptr == str)
        return -1;
    return num;
}

static int json_atoi_err(char *str, int *buf)
{
    char *endptr;

    *buf = strtol(str, &endptr, 0);
    if (endptr == str)
        return -1;
    return 1;
}

/* create string of n amount of spaces */
static void get_spaces(char *buf, uint8_t spaces) {
    uint8_t i;
    for (i=0 ; i<spaces ; i++) {
        buf[i] = ' ';
    }
    buf[i] = '\0';
}

/* print context for error message */
static void print_error(struct Position *pos, uint32_t amount) {
    char lctext[2*LINES_CONTEXT];       // buffer for string left from current char
    char rctext[2*LINES_CONTEXT];       // buffer for string right from current char

    char *lptr = lctext;
    char *rptr = rctext;

    // get context
    for (uint8_t i=0,j=amount ; i<amount ; i++, j--) {

        // check if we go out of left string bounds
        if ((int8_t)(pos->npos - j) >= 0) {
            *lptr = *(pos->c - j);                  // add char to string
            lptr++;
        }
        // check if we go out of right string bounds
        // BUG this is not bugfree
        if ((pos->npos + i +1) < strlen(pos->json)) {
            *rptr = *(pos->c + i +1);               // add char to string
            rptr++;
        }
    }
    rctext[amount] = '\0';
    lctext[amount] = '\0';

    printf("JSON syntax error: >%c< @ (%d,%d)\n", *(pos->c), pos->rows, pos->cols);
    printf("%s%s%c%s<--%s%s\n", lctext, JRED, *(pos->c), JBLUE, JRESET, rctext);
}

static char* pos_next(struct Position *pos)
{
    /* Increment struct position in json string */
    // keep track of rows/cols position
    if (*(pos->c) == '\n') {
        pos->rows += 1;
        pos->cols = 0;
    } else {
        (pos->cols)++;
    }
    (pos->npos)++;
    (pos->c)++;

    // NOTE: EOF should not be reached under normal conditions.
    //       This indicates corrupted JSON
    if (pos->npos >= pos->length) {
        // check if character is printable
        if (*(pos->c) >= 32 && *(pos->c) <= 126)
            debug("EOL @ c=%c, pos: %d, cxr: %dx%d\n", *(pos->c), pos->npos, pos->cols, pos->rows);
        //else
        //    debug("EOL @ (unprintable), pos: %d, cxr: %dx%d\n", pos->npos, pos->cols, pos->rows);
        return NULL;
    }
    return pos->c;
}

static char fforward_skip_escaped(struct Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char* buf)
{
    /* fast forward until a char from search_lst is found
     * Save all chars in buf until a char from search_lst is found
     * Only save in buf when a char is found in expected_lst
     * Error is a char from unwanted_lst is found
     *
     * If buf == NULL,          don't save chars
     * If expected_lst == NULL, allow all characters
     * If unwanted_lst == NULL, allow all characters
     */
    // TODO char can not be -1

    // save skipped chars that are on expected_lst in buffer
    char* ptr = buf;

    // don't return these chars with buffer
    ignore_lst = (ignore_lst) ? ignore_lst : "";
    unwanted_lst = (unwanted_lst) ? unwanted_lst : "";
    //printf("looking for %s in %s\n", search_lst, pos->c);

    while (1) {
        if (strchr(search_lst, *(pos->c))) {
            // check if previous character whas a backslash which indicates escaped
            if (pos->npos > 0 && *(pos->c-1) == '\\') {
                //printf("ignoring escaped: %c, %c\n", *(pos->c-1), *(pos->c));
            }
            else
                break;
            //printf("found char: %c\n", *(pos->c));
        }
        if (strchr(unwanted_lst, *(pos->c)))
            return -1;

        if (expected_lst != NULL) {
            if (!strchr(expected_lst, *(pos->c)))
                return -1;
        }
        if (buf != NULL && !strchr(ignore_lst, *(pos->c)))
            *ptr++ = *(pos->c);

        pos_next(pos);
    }
    // terminate string
    if (ptr != NULL)
        *ptr = '\0';

    char ret = *(pos->c);

    //if (buf)
    //    printf("!!!!!!!!!!: >%s<\n", buf);
    return ret;
}

static int count_backslashes(struct Position *pos)
{
    int count = 0;

    while (*(pos->c - (count+1)) == '\\' && pos->npos >= count)
        count++;

    return count;
}

static char fforward_skip_escaped_grow(struct Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char** buf)
{
    /* fast forward until a char from search_lst is found
     * Save all chars in buf until a char from search_lst is found
     * Only save in buf when a char is found in expected_lst
     * Error is a char from unwanted_lst is found
     *
     * If buf == NULL,          don't save chars
     * If expected_lst == NULL, allow all characters
     * If unwanted_lst == NULL, allow all characters
     */
    // TODO char can not be -1

    // save skipped chars that are on expected_lst in buffer
    int grow_amount = 256;
    int buf_size = 0;
    int buf_pos = 0;

    if (buf != NULL) {
        buf_size = grow_amount;
        *buf = malloc(buf_size);
    }

    // don't return these chars with buffer
    ignore_lst = (ignore_lst) ? ignore_lst : "";
    unwanted_lst = (unwanted_lst) ? unwanted_lst : "";

    while (1) {
        if (buf != NULL && buf_pos >= buf_size) {
            buf_size += grow_amount;
            *buf = realloc(*buf, buf_size+1);
        }

        if (strchr(search_lst, *(pos->c))) {
            // if a character is found that is on the search list we have to check
            // if the character is escaped. So we need to look back to check for
            // an uneven amount of backslashes
            if (count_backslashes(pos) % 2 == 0)
                break;
        }
        if (strchr(unwanted_lst, *(pos->c))) {
            goto cleanup_on_err;
        }

        if (expected_lst != NULL) {
            if (!strchr(expected_lst, *(pos->c)))
                goto cleanup_on_err;
        }
        if (buf != NULL && !strchr(ignore_lst, *(pos->c)))
            (*buf)[buf_pos] = *(pos->c);

        pos_next(pos);
        buf_pos++;
    }
    // terminate string
    if (buf != NULL)
        (*buf)[buf_pos] = '\0';

    char ret = *(pos->c);
    return ret;

cleanup_on_err:
    if (buf != NULL)
        free(buf);
    return -1;
}

static void json_object_add_child(struct JSONObject *parent, struct JSONObject *child)
{
    /* we are not creating array here but in json_parse_object().
     * So we should either come up with a solution or drop the functionality
     */
    parent->length++;
    child->parent = parent;

    // value is headl node
    if (parent->value == NULL) {

        // child is first node in linked list
        parent->value = child;
        child->prev = NULL;
        child->next = NULL;
        child->index = 0;
    }
    else {
        // add to end of linked list
        struct JSONObject *prev = parent->value;

        // get last item in ll
        while (prev->next != NULL)
            prev = prev->next;

        prev->next = child;
        child->prev = prev;
        child->next = NULL;
        child->index = child->prev->index + 1;
    }
}

static enum JSONStatus json_parse_number(struct JSONObject* jo, struct Position* pos)
{
    /* All numbers are floats */
    char tmp[MAX_BUF] = {'\0'};
    char c;

    jo->dtype = JSON_NUMBER;
    jo->is_number = true;

    if ((c = fforward_skip_escaped(pos, ", ]}\n", "0123456789-null.", NULL, "\n", tmp)) < 0) {
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
    //jo->value = strdup(tmp);
    double* value = malloc(sizeof(double));

    // need to set locale because atof needs to know what char is the decimal points/comma
    setlocale(LC_NUMERIC,"C");

    *value = atof(tmp);
    jo->value = value;
    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse_bool(struct JSONObject* jo, struct Position* pos)
{
    char tmp[MAX_BUF] = {'\0'};
    char c;

    jo->dtype = JSON_BOOL;
    jo->is_bool = true;

    if ((c = fforward_skip_escaped(pos, ", ]}\n", "truefalse", NULL, "\n", tmp)) < 0) {
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
    bool* value = malloc(sizeof(bool));

    if (strcmp(tmp, "true") == 0)
        *value = true;
    else if (strcmp(tmp, "false") == 0)
        *value = false;
    else
        return PARSE_ERROR;

    jo->value = value;
    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse_key(struct JSONObject* jo, struct Position* pos)
{
    /* Parse key part of an object */
    char c;

    // skip to start of key
    if ((c = fforward_skip_escaped(pos, "\"'}", ", \n\t", NULL, "\n", NULL)) < 0) {
        printf("error\n");
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
    if (c == '}') {
        pos_next(pos);
        return END_OF_OBJECT;
    }

    pos_next(pos);

    // read key
    if ((c = fforward_skip_escaped_grow(pos, "\"'", NULL, NULL, "\n", &(jo->key))) < 0) {
        printf("Error while parsing key\n");
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
    //jo->key = key;
    if (c == '}') {
        pos_next(pos);
        return END_OF_OBJECT;
    }
    
    // skip over "
    pos_next(pos);

    // find colon
    if ((c = fforward_skip_escaped(pos, ":", " \n", NULL, "\n", NULL)) < 0) {
        printf("Error while parsing key\n");
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }

    // skip over colon
    pos_next(pos);

    //printf("json read key: %s\n", key);
    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse_string(struct JSONObject* jo, struct Position* pos, char quote_chr)
{
    char c;

    jo->dtype = JSON_STRING;
    jo->is_string = true;

    // look for closing quotes, quote_chr tells us if it is " or ' that we're looking for
    if (quote_chr == '\'') {
        if ((c = fforward_skip_escaped_grow(pos, "'\n", NULL, NULL, "\n", (char**)&(jo->value))) < 0) {
            printf("Error while parsing string, Failed to find closing quotes\n");
            print_error(pos, LINES_CONTEXT);
            return PARSE_ERROR;
        }
    }
    else if (quote_chr == '"') {
        if ((c = fforward_skip_escaped_grow(pos, "\"\n", NULL, NULL, "\n", (char**)&(jo->value))) < 0) {
            printf("Error while parsing string, Failed to find closing quotes\n");
            print_error(pos, LINES_CONTEXT);
            return PARSE_ERROR;
        }
    }
    else {
        return PARSE_ERROR;
    }

    //printf("json read string: %s\n", tmp);

    //if ((c = fforward_skip_escaped(pos, "\"'\n", NULL, NULL, "\n", tmp)) < 0) {
    //    printf("Error while parsing string, Failed to find closing quotes\n");
    //    print_error(pos, LINES_CONTEXT);
    //    return PARSE_ERROR;
    //}
    //jo->value = tmp;


    // step over " char
    pos_next(pos);

    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse_array(struct JSONObject* jo, struct Position* pos)
{
    jo->dtype = JSON_ARRAY;
    //jo->length = 0;
    jo->is_array = true;

    while (1) {
        struct JSONObject* child = json_object_init(jo);

        enum JSONStatus ret = json_parse(child, pos);
        if (ret < 0) {
            json_obj_destroy(child);
            return ret;
        }
        else if (ret == END_OF_ARRAY) {
            json_obj_destroy(child);
            break;
        }

        // look for comma or array end
        if (fforward_skip_escaped(pos, ",]", "\n\t ", NULL, "\n", NULL) < 0) {
            printf("Error while parsing array\n");
            print_error(pos, LINES_CONTEXT);
            json_obj_destroy(child);
            return PARSE_ERROR;
        }
    }
    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse_object(struct JSONObject* jo, struct Position* pos)
{
    jo->dtype = JSON_OBJECT;
    jo->is_object = true;

    while (1) {
        struct JSONObject* child = json_object_init(jo);
        enum JSONStatus ret_key = json_parse_key(child, pos);

        if (ret_key < 0) {
            json_obj_destroy(child);
            return ret_key;
        }
        else if (ret_key == END_OF_OBJECT) {
            json_obj_destroy(child);
            break;
        }

        // parse the value
        enum JSONStatus ret_value = json_parse(child, pos);
        if (ret_value != STATUS_SUCCESS) {
            json_obj_destroy(child);
            return PARSE_ERROR;
        }

        // look for comma or object end
        if (fforward_skip_escaped(pos, ",}", "\n\t ", NULL, "\n", NULL) < 0) {
            printf("Error while parsing object\n");
            print_error(pos, LINES_CONTEXT);
            json_obj_destroy(child);
            return PARSE_ERROR;
        }
    }
    return STATUS_SUCCESS;
}

static enum JSONStatus json_parse(struct JSONObject* jo, struct Position* pos)
{
    char tmp[MAX_BUF] = {'\0'};
    char c;

    // detect type
    if ((c = fforward_skip_escaped(pos, "\"[{1234567890-n.tf}]", NULL, NULL, "\n", tmp)) < 0) {
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }

    if (c == '[') {
        pos_next(pos);
        return json_parse_array(jo, pos);
    }
    else if (c == ']') {
        pos_next(pos);
        return END_OF_ARRAY;
    }
    else if (c == '{') {
        pos_next(pos);
        return json_parse_object(jo, pos);
    }
    else if (c == '}') {
        pos_next(pos);
        return END_OF_OBJECT;
    }
    //else if ((c == '"' || c == '\'') &&  *(pos->c-1) != '\\') {
    else if (c == '"' || c == '\'') {
        pos_next(pos);
        return json_parse_string(jo, pos, c);
    }
    else if (strchr("0123456789-n.", c)) {
        return json_parse_number(jo, pos);
    }
    else if (strchr("tf", c)) {
        return json_parse_bool(jo, pos);
    }
    else {
        return STATUS_ERROR;
    }
}

struct JSONObject* json_load(char* buf)
{
    struct Position* pos = malloc(sizeof(struct Position));
    pos->json     = buf;
    pos->c        = buf;
    pos->npos     = 0;
    pos->cols     = 1;
    pos->rows     = 1;
    pos->length   = strlen(buf);

    struct JSONObject* root = json_object_init(NULL);
    enum JSONStatus ret = json_parse(root, pos);

    // cleanup
    //free(pos->json);
    free(pos);

    return (ret < 0) ? NULL : root;
}

/* read file from disk and parse JSON */
struct JSONObject* json_load_file(char *path)
{
    // read file in chunks and dynamically allocate memory for buffer
    uint32_t chunk_size = 1000;   // read file in chunks
    uint32_t offset     = 0;    // offset in buffer to write data to
    uint32_t n_read     = 0;    // store amount of chars read from file
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        printf("JSON: File doesn't exist: %s\n", path);
        return NULL;
    }

    char *buf = calloc(chunk_size, 1);

    while ((n_read=fread(buf + offset, 1, chunk_size, fp)) > 0) {
        offset += n_read;
        buf = realloc(buf, offset+chunk_size);
    }

    buf[offset] = '\0';     // properly end string array
    fclose(fp);
    printf("read: %ld bytes\n", strlen(buf));

    struct JSONObject* jo = json_load(buf);
    return jo;
}

char* json_object_to_string(struct JSONObject *jo, int spaces)
{
    char *buf = NULL;
    json_object_to_string_rec(jo, &buf, 0, spaces);
    return buf;
}

int json_object_to_file(struct JSONObject *jo, char *path, int spaces)
{
    char *buf = json_object_to_string(jo, spaces);
    FILE *fp = fopen(path, "wb");

    if (fp == NULL) {
        printf("Failed to open file for writing\n");
        return -1;
    }
    size_t n;
    if ((n = fwrite(buf, 1, strlen(buf), fp)) < strlen(buf)) {
        printf("Failed to write to file (%ld bytes written)\n", n);
        return -1;
    }
    fclose(fp);
    return 0;
}

struct JSONObject* json_object_init(struct JSONObject* parent)
{
    struct JSONObject* jo = malloc(sizeof(struct JSONObject));
    jo->parent = parent;
    jo->prev = NULL;
    jo->next = NULL;

    jo->length = 0;
    jo->index = -1;

    jo->dtype = JSON_UNKNOWN;

    jo->key = NULL;
    jo->value = NULL;

    jo->is_string = false;
    jo->is_number  = false;
    jo->is_bool   = false;
    jo->is_array  = false;
    jo->is_object = false;

    if (parent != NULL && (parent->is_array || parent->is_object))
        json_object_add_child(parent, jo);

    return jo;
}

void json_obj_destroy(struct JSONObject* jo)
{
    /* TODO when removing an array index the indexes are not continuous anymore so 
     * they should be rebuilt
     */

    if (jo->key != NULL)
        free(jo->key);

    if (jo->parent != NULL && (jo->parent->is_array || jo->parent->is_object)) {

        // remove child from linked list
        if (jo->prev && jo->next) {
            jo->prev->next = jo->next;
            jo->next->prev = jo->prev->next;
        }
        else if (jo->prev) {
            jo->prev->next = NULL;
        }
        else if (jo->next) {
            jo->next->prev = NULL;
        }
        else {
            // no more linked list
            jo->parent->value = NULL;
        }

        
        if (jo->parent->length > 0)
            jo->parent->length--;
    }

    if (jo->dtype == JSON_OBJECT || jo->dtype == JSON_ARRAY) {

        struct JSONObject* child = jo->value;
        while (child != NULL) {
            struct JSONObject* tmp = child->next;
            json_obj_destroy(child);
            child = tmp;
        }
    }
    else {
        free(jo->value);
    }

    free(jo);
}

void json_print(struct JSONObject* jo, uint32_t level)
{
    uint8_t incr = 3;
    char space[level+1];
    get_spaces(space, level);

    if (jo != NULL) {
        debug("%s", space);
        if (jo->parent && jo->index >= 0 && jo->parent->dtype == JSON_ARRAY)
            debug("%s%d:%s ", JCOL_ARR_INDEX, jo->index, JRESET);
        if (jo->key)
            debug("%s%s:%s ", JCOL_KEY, jo->key, JRESET);

        switch (jo->dtype) {

            case JSON_NUMBER:
                debug("%s%f%s\n", JCOL_NUM, json_get_number(jo), JRESET);
                break;

            case JSON_STRING:
                debug("%s\"%s\"%s\n", JCOL_STR, json_get_string(jo), JRESET);
                break;

            case JSON_BOOL:
                debug("%s%s%s\n", JCOL_BOOL, json_get_bool(jo) ? "true" : "false", JRESET);
                break;

            case JSON_ARRAY:
                debug("%s[ARRAY]%s\n", JCOL_ARR, JRESET);
                json_print(jo->value, level+incr);
                break;

            case JSON_OBJECT:
                debug("%s[OBJECT]%s\n", JCOL_OBJ, JRESET);
                json_print(jo->value, level+incr);
                break;

            case JSON_UNKNOWN:
                debug("%s[UNKNOWN]%s\n", JCOL_UNKNOWN, JRESET);
                break;
        }

        if (jo->next != NULL)
            json_print(jo->next, level);
    }
}

double json_get_number(struct JSONObject* jo)
{
    return *((double*)jo->value);
}

char* json_get_string(struct JSONObject* jo)
{
    return (char*)jo->value;
}

bool json_get_bool(struct JSONObject* jo)
{
    return *((bool*)jo->value);
}

struct JSONObject *json_object_init_string(struct JSONObject *parent, const char *key, const char *value)
{
    struct JSONObject *jo = json_object_init(parent);
    jo->dtype = JSON_STRING;
    jo->is_string = 1;
    jo->value = strdup(value);
    if (key)
        jo->key = strdup(key);
    return jo;
}

struct JSONObject *json_object_init_object(struct JSONObject *parent, const char *key)
{
    struct JSONObject *jo = json_object_init(parent);
    jo->dtype = JSON_OBJECT;
    jo->is_object = 1;
    if (key)
        jo->key = strdup(key);
    return jo;
}

struct JSONObject *json_object_init_array(struct JSONObject *parent, const char *key)
{
    struct JSONObject *jo = json_object_init(parent);
    jo->dtype = JSON_ARRAY;
    jo->is_array = 1;
    if (key)
        jo->key = strdup(key);
    return jo;
}

int is_array_index(char *string)
{
    /* Check if string contains an array index by following the format:
     *   [123] => is index,               return: 123
     *   [-1]  => last item,              return:  -1
     *   [?]   => append to end of array, return:  -2
     *   else  => failed to parse,        return:  -3
     *
     */
    int len = strlen(string);
    int ret = JSON_ARR_INDEX_ERROR;
    int index;

    if (len == 0)
        return JSON_ARR_INDEX_ERROR;

    if (string[0] != '[' && string[len-1] != ']')
        return JSON_ARR_INDEX_ERROR;

    char *buf = malloc(len +1);
    char *pbuf = buf;

    for (int i=1 ; i<len-1 ; i++, pbuf++)
        *pbuf = string[i];
    *pbuf = '\0';

    if (strncmp(buf, "?", len) == 0) {
        ret = JSON_ARR_INDEX_APPEND;
    }

    else if (json_atoi_err(buf, &index) >= 0) {
        if (*buf == -1)
            ret = JSON_ARR_INDEX_LAST;
        else
            ret = index;
    }

    free(buf);
    return ret;

}

struct JSONObject* json_get_path(struct JSONObject *rn, char *buf)
{
    /* Find a path given as a string and return node if found */
    if (rn == NULL)
        return NULL;

    if (!(rn->is_object || rn->is_array))
        return NULL;

    char path[256] = "";
    strncpy(path, buf, strlen(buf));

    struct JSONObject* seg = rn;
    char *lasts;
    char *token = strtok_r(path, PATH_DELIM, &lasts);

    while(token) {
        seg = seg->value;

        while (1) {
            if (seg == NULL)
                return NULL;

            //if (seg->parent != NULL && seg->parent->is_array && json_atou_err(token) >= 0 && seg->index == json_atou_err(token))
            if (seg->parent != NULL && seg->parent->is_array && is_array_index(token) == JSON_ARR_INDEX_APPEND)
                return NULL;

            if (seg->parent != NULL && seg->parent->is_array && is_array_index(token) == JSON_ARR_INDEX_LAST) {
                if (seg->index == seg->length-1) {
                    printf("returning last item in array\n");
                    break;
                }
            }

            if (seg->parent != NULL && seg->parent->is_array && seg->index == is_array_index(token))
                break;

            if (seg->key != NULL && strncmp(token, seg->key, strlen(token)) == 0)
                break;


            seg = seg->next;
        }

        token = strtok_r(NULL, PATH_DELIM, &lasts);
    }
    return seg;
}

struct JSONObject *json_set_path(struct JSONObject *rn, const char *buf, struct JSONObject *child)
{
    /* Set child as a child in object or array at end of path.
     * Array items can be numbers or '?' to append to end of list */
    if (rn == NULL)
        return NULL;

    //printf(">> %d\n", is_array_index("[234]"));
    //printf(">> %d\n", is_array_index("[?]"));
    //printf(">> %d\n", is_array_index("[?234]"));
    //printf(">> %d\n", is_array_index("xxx]"));

    char path[256] = "";
    strncpy(path, buf, strlen(buf));

    struct JSONObject* seg = rn;

    char *lasts;
    char *token = strtok_r(path, PATH_DELIM, &lasts);

    while(token) {
        int index;

        // check if token already exists as index or key
        struct JSONObject *tmp = json_get_path(seg, token);
        if (tmp != NULL)
            seg = tmp;

        // token is an array index
        else if ((index = is_array_index(token)) >= JSON_ARR_INDEX_APPEND) {
            printf("index: %d\n", index);
            if (seg != NULL && seg->is_object && seg->length == 0) {
                // TODO check if index exists in array and replace if so
                // turn empty parent object into an array
                seg->dtype = JSON_ARRAY;
                seg->is_array = 1;
                seg->is_object = 0;
            }

            if (seg != NULL && !seg->is_array) {
                printf("ERROR: parent is not an aray\n");
                return NULL;
            }
        }

        // token is a key
        else {
            printf("New object: %s\n", token);
            seg = json_object_init_object(seg, NULL);

            if (seg->parent->is_object)
                seg->key = strdup(token);
        }
        token = strtok_r(NULL, PATH_DELIM, &lasts);
    }

    if (seg != NULL && seg->is_array && child->key != NULL) {
        printf("ERROR: can not add k:v to array, key=%s\n", child->key);
        return NULL;
    }

    // last segment in path should hold the value
    json_object_add_child(seg, child);

    return child;
}
