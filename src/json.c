#include "json.h"

void debug(char* fmt, ...)
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

/* create string of n amount of spaces */
void get_spaces(char *buf, uint8_t spaces) {
    uint8_t i;
    for (i=0 ; i<spaces ; i++) {
        buf[i] = ' ';
    }
    buf[i] = '\0';
}

/* print context for error message */
void print_error(Position *pos, uint32_t amount) {
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

    printf("JSON syntax error: %c @ (%d,%d)\n", *(pos->c), pos->rows, pos->cols);
    printf("%s%s%c%s<--%s%s\n", lctext, JRED, *(pos->c), JBLUE, JRESET, rctext);
}

void json_print(JSONObject* jo, uint32_t level)
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
        //printf("key: %s, dtype = %d\n", jo->key, jo->dtype);

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

double json_get_number(JSONObject* jo)
{
    return *((double*)jo->value);
}

char* json_get_string(JSONObject* jo)
{
    return (char*)jo->value;
}

bool json_get_bool(JSONObject* jo)
{
    return *((bool*)jo->value);
}

/* read file from disk and parse JSON */
JSONObject* json_load_file(char *path)
{
    // read file in chunks and dynamically allocate memory for buffer
    uint32_t chunk_size = 1000;   // read file in chunks
    uint32_t offset     = 0;    // offset in buffer to write data to
    uint32_t n_read     = 0;    // store amount of chars read from file
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        printf("File doesn't exist\n");
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

    JSONObject* jo = json_load(buf);
    return jo;
}

char* pos_next(Position *pos)
{
    /* Increment position in json string */
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
        else
            debug("EOL @ (unprintable), pos: %d, cxr: %dx%d\n", pos->npos, pos->cols, pos->rows);
        return NULL;
    }
    return pos->c;
}

char fforward(Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char* buf)
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

    while (!strchr(search_lst, *(pos->c))) {
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

    return ret;
}

char fforward_skip_escaped(Position* pos, char* search_lst, char* expected_lst, char* unwanted_lst, char* ignore_lst, char* buf)
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

JSONObject* json_object_init(JSONObject* parent)
{
    JSONObject* jo = malloc(sizeof(JSONObject));
    jo->parent = parent;
    jo->children = NULL;
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
    return jo;
}

void json_obj_destroy(JSONObject* jo)
{
    if (jo->dtype == JSON_OBJECT)
        free(jo->key);

    if (jo->dtype == JSON_OBJECT || jo->dtype == JSON_ARRAY) {

        JSONObject* child = jo->value;
        while (child != NULL) {
            JSONObject* tmp = child->next;
            json_obj_destroy(child);
            child = tmp;
        }
        free(jo->children);
    } else {
        free(jo->value);
    }

    free(jo);
}

JSONStatus json_parse_number(JSONObject* jo, Position* pos)
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

JSONStatus json_parse_bool(JSONObject* jo, Position* pos)
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

JSONStatus json_parse_key(JSONObject* jo, Position* pos)
{
    /* Parse key part of an object */
    char key[MAX_BUF] = {'\0'};
    char c;

    // skip to start of key
    if ((c = fforward_skip_escaped(pos, "\"'}", ", \n", NULL, "\n", NULL)) < 0) {
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
    if (c == '}') {
        pos_next(pos);
        return END_OF_OBJECT;
    }

    pos_next(pos);

    // read key
    if ((c = fforward_skip_escaped(pos, "\"'", NULL, NULL, "\n", key)) < 0) {
        printf("Error while parsing key\n");
        print_error(pos, LINES_CONTEXT);
        return PARSE_ERROR;
    }
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

    jo->key = strdup(key);
    return STATUS_SUCCESS;
}

JSONStatus json_parse_string(JSONObject* jo, Position* pos, char quote_chr)
{
    char tmp[MAX_BUF] = {'\0'};
    char c;

    jo->dtype = JSON_STRING;
    jo->is_string = true;

    // look for closing quotes, quote_chr tells us if it is " or ' that we're looking for
    if (quote_chr == '\'') {
        if ((c = fforward_skip_escaped(pos, "'\n", NULL, NULL, "\n", tmp)) < 0) {
            printf("Error while parsing string, Failed to find closing quotes\n");
            print_error(pos, LINES_CONTEXT);
            return PARSE_ERROR;
        }
    }
    else if (quote_chr == '"') {
        if ((c = fforward_skip_escaped(pos, "\"\n", NULL, NULL, "\n", tmp)) < 0) {
            printf("Error while parsing string, Failed to find closing quotes\n");
            print_error(pos, LINES_CONTEXT);
            return PARSE_ERROR;
        }
    }
    else {
        return PARSE_ERROR;
    }

    //if ((c = fforward_skip_escaped(pos, "\"'\n", NULL, NULL, "\n", tmp)) < 0) {
    //    printf("Error while parsing string, Failed to find closing quotes\n");
    //    print_error(pos, LINES_CONTEXT);
    //    return PARSE_ERROR;
    //}
    jo->value = strdup(tmp);


    // step over " char
    pos_next(pos);

    return STATUS_SUCCESS;
}

JSONStatus json_parse_array(JSONObject* jo, Position* pos)
{
    jo->dtype = JSON_ARRAY;
    jo->length = 0;
    jo->is_array = true;

    JSONObject* head = NULL;
    JSONObject* tail = NULL;

    while (1) {
        JSONObject* child = json_object_init(jo);

        JSONStatus ret = json_parse(child, pos);
        if (ret < 0) {
            json_obj_destroy(child);
            return ret;
        }
        else if (ret == END_OF_ARRAY) {
            json_obj_destroy(child);
            break;
        }

        // look for comma or array end
        if (fforward_skip_escaped(pos, ",]", "\n ", NULL, "\n", NULL) < 0) {
            printf("Error while parsing array\n");
            print_error(pos, LINES_CONTEXT);
            json_obj_destroy(child);
            return PARSE_ERROR;
        }

        if (head == NULL) {
            head = child;
            tail = child;
        } else {
            JSONObject* prev = tail;
            prev->next = child;
            child->prev = prev;
            tail = child;
        }
        jo->length++;
    }
    jo->value = head;
    //tail->next = NULL;

    // we know the index length now so lets create the array
    jo->children = malloc(jo->length * sizeof(JSONObject));
    JSONObject* child = jo->value;
    for (int i=0 ; i<jo->length ; i++) {
        jo->children[i] = child;
        child->index = i;
        child = child->next;
    }

    return STATUS_SUCCESS;
}

JSONStatus json_parse_object(JSONObject* jo, Position* pos)
{
    jo->dtype = JSON_OBJECT;
    jo->length = 0;
    jo->is_object = true;

    JSONObject* head = NULL;
    JSONObject* tail = NULL;

    while (1) {
        JSONObject* child = json_object_init(jo);
        JSONStatus ret_key = json_parse_key(child, pos);

        if (ret_key < 0) {
            json_obj_destroy(child);
            return ret_key;
        }
        else if (ret_key == END_OF_OBJECT) {
            json_obj_destroy(child);
            break;
        }

        // parse the value
        JSONStatus ret_value = json_parse(child, pos);
        if (ret_value != STATUS_SUCCESS) {
            json_obj_destroy(child);
            return PARSE_ERROR;
        }

        // look for comma or object end
        if (fforward_skip_escaped(pos, ",}", "\n ", NULL, "\n", NULL) < 0) {
            printf("Error while parsing object\n");
            print_error(pos, LINES_CONTEXT);
            json_obj_destroy(child);
            return PARSE_ERROR;
        }

        if (head == NULL) {
            head = child;
            tail = child;
        } else {
            JSONObject* prev = tail;
            prev->next = child;
            child->prev = prev;
            tail = child;
        }
        jo->length++;

    }
    jo->value = head;
    //tail->next = NULL;

    // we know the index length now so lets create the array
    jo->children = malloc(jo->length * sizeof(JSONObject));
    JSONObject* child = jo->value;
    for (int i=0 ; i<jo->length ; i++) {
        jo->children[i] = child;
        child->index = i;
        child = child->next;
    }

    return STATUS_SUCCESS;
}

JSONStatus json_parse(JSONObject* jo, Position* pos)
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

JSONObject* json_load(char* buf)
{
    Position* pos = malloc(sizeof(Position));
    pos->json     = buf;
    pos->c        = buf;
    pos->npos     = 0;
    pos->cols     = 1;
    pos->rows     = 1;
    pos->length   = strlen(buf);

    JSONObject* root = json_object_init(NULL);
    JSONStatus ret = json_parse(root, pos);

    // cleanup
    //free(pos->json);
    free(pos);

    return (ret < 0) ? NULL : root;
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


    printf("looking for path: %s\n", path);

    JSONObject* seg = rn;
    char *token = strtok(path, PATH_DELIM);

    while(token) {
        seg = seg->value;

        while (1) {
            if (seg == NULL)
                return NULL;

            if (strncmp(token, seg->key, strlen(token)) == 0)
                break;

            seg = seg->next;
        }

        token = strtok(NULL, PATH_DELIM);
    }
    return seg;

}
