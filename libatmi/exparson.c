/*
 Exparson
 based on parson ( http://kgabis.github.com/parson/ )
 Copyright (c) 2012 - 2017 Krzysztof Gabis

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif /* _CRT_SECURE_NO_WARNINGS */
#endif /* _MSC_VER */

#include "exparson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

/* Apparently sscanf is not implemented in some "standard" libraries, so don't use it, if you
 * don't have to. */
#define sscanf THINK_TWICE_ABOUT_USING_SSCANF

#define STARTING_CAPACITY 16
#define MAX_NESTING       2048
#define FLOAT_FORMAT      "%1.17g"

#define SIZEOF_TOKEN(a)       (sizeof(a) - 1)
#define SKIP_CHAR(str)        ((*str)++)
#define SKIP_WHITESPACES(str) while (isspace(**str)) { SKIP_CHAR(str); }
#define MAX(a, b)             ((a) > (b) ? (a) : (b))

#undef malloc
#undef free

static EXJSON_Malloc_Function exparson_malloc = malloc;
static EXJSON_Free_Function exparson_free = free;

#define IS_CONT(b) (((unsigned char)(b) & 0xC0) == 0x80) /* is utf-8 continuation byte */

/* Type definitions */
typedef union exjson_value_value {
    char        *string;
    double       number;
    EXJSON_Object *object;
    EXJSON_Array  *array;
    int          boolean;
    int          null;
} EXJSON_Value_Value;

struct exjson_value_t {
    EXJSON_Value      *parent;
    EXJSON_Value_Type  type;
    EXJSON_Value_Value value;
};

struct exjson_object_t {
    EXJSON_Value  *wrapping_value;
    char       **names;
    EXJSON_Value **values;
    size_t       count;
    size_t       capacity;
};

struct exjson_array_t {
    EXJSON_Value  *wrapping_value;
    EXJSON_Value **items;
    size_t       count;
    size_t       capacity;
};

/* Various */
static char * read_file(const char *filename);
static void   remove_comments(char *string, const char *start_token, const char *end_token);
static char * exparson_strndup(const char *string, size_t n);
static char * exparson_strdup(const char *string);
static int    hex_char_to_int(char c);
static int    parse_utf16_hex(const char *string, unsigned int *result);
static int    num_bytes_in_utf8_sequence(unsigned char c);
static int    verify_utf8_sequence(const unsigned char *string, int *len);
static int    is_valid_utf8(const char *string, size_t string_len);
static int    is_decimal(const char *string, size_t length);

/* EXJSON Object */
static EXJSON_Object * exjson_object_init(EXJSON_Value *wrapping_value);
static EXJSON_Status   exjson_object_add(EXJSON_Object *object, const char *name, EXJSON_Value *value);
static EXJSON_Status   exjson_object_resize(EXJSON_Object *object, size_t new_capacity);
static EXJSON_Value  * exjson_object_nget_value(const EXJSON_Object *object, const char *name, size_t n);
static void          exjson_object_free(EXJSON_Object *object);

/* EXJSON Array */
static EXJSON_Array * exjson_array_init(EXJSON_Value *wrapping_value);
static EXJSON_Status  exjson_array_add(EXJSON_Array *array, EXJSON_Value *value);
static EXJSON_Status  exjson_array_resize(EXJSON_Array *array, size_t new_capacity);
static void         exjson_array_free(EXJSON_Array *array);

/* EXJSON Value */
static EXJSON_Value * exjson_value_init_string_no_copy(char *string);

/* Parser */
static EXJSON_Status  skip_quotes(const char **string);
static int          parse_utf16(const char **unprocessed, char **processed);
static char *       process_string(const char *input, size_t len);
static char *       get_quoted_string(const char **string);
static EXJSON_Value * parse_object_value(const char **string, size_t nesting);
static EXJSON_Value * parse_array_value(const char **string, size_t nesting);
static EXJSON_Value * parse_string_value(const char **string);
static EXJSON_Value * parse_boolean_value(const char **string);
static EXJSON_Value * parse_number_value(const char **string);
static EXJSON_Value * parse_null_value(const char **string);
static EXJSON_Value * parse_value(const char **string, size_t nesting);

/* Serialization */
static int    exjson_serialize_to_buffer_r(const EXJSON_Value *value, char *buf, int level, int is_pretty, char *num_buf);
static int    exjson_serialize_string(const char *string, char *buf);
static int    append_indent(char *buf, int level);
static int    append_string(char *buf, const char *string);

/* Various */
static char * exparson_strndup(const char *string, size_t n) {
    char *output_string = (char*)exparson_malloc(n + 1);
    if (!output_string) {
        return NULL;
    }
    output_string[n] = '\0';
    strncpy(output_string, string, n);
    return output_string;
}

static char * exparson_strdup(const char *string) {
    return exparson_strndup(string, strlen(string));
}

static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int parse_utf16_hex(const char *s, unsigned int *result) {
    int x1, x2, x3, x4;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0' || s[3] == '\0') {
        return 0;
    }
    x1 = hex_char_to_int(s[0]);
    x2 = hex_char_to_int(s[1]);
    x3 = hex_char_to_int(s[2]);
    x4 = hex_char_to_int(s[3]);
    if (x1 == -1 || x2 == -1 || x3 == -1 || x4 == -1) {
        return 0;
    }
    *result = (unsigned int)((x1 << 12) | (x2 << 8) | (x3 << 4) | x4);
    return 1;
}

static int num_bytes_in_utf8_sequence(unsigned char c) {
    if (c == 0xC0 || c == 0xC1 || c > 0xF4 || IS_CONT(c)) {
        return 0;
    } else if ((c & 0x80) == 0) {    /* 0xxxxxxx */
        return 1;
    } else if ((c & 0xE0) == 0xC0) { /* 110xxxxx */
        return 2;
    } else if ((c & 0xF0) == 0xE0) { /* 1110xxxx */
        return 3;
    } else if ((c & 0xF8) == 0xF0) { /* 11110xxx */
        return 4;
    }
    return 0; /* won't happen */
}

static int verify_utf8_sequence(const unsigned char *string, int *len) {
    unsigned int cp = 0;
    *len = num_bytes_in_utf8_sequence(string[0]);

    if (*len == 1) {
        cp = string[0];
    } else if (*len == 2 && IS_CONT(string[1])) {
        cp = string[0] & 0x1F;
        cp = (cp << 6) | (string[1] & 0x3F);
    } else if (*len == 3 && IS_CONT(string[1]) && IS_CONT(string[2])) {
        cp = ((unsigned char)string[0]) & 0xF;
        cp = (cp << 6) | (string[1] & 0x3F);
        cp = (cp << 6) | (string[2] & 0x3F);
    } else if (*len == 4 && IS_CONT(string[1]) && IS_CONT(string[2]) && IS_CONT(string[3])) {
        cp = string[0] & 0x7;
        cp = (cp << 6) | (string[1] & 0x3F);
        cp = (cp << 6) | (string[2] & 0x3F);
        cp = (cp << 6) | (string[3] & 0x3F);
    } else {
        return 0;
    }

    /* overlong encodings */
    if ((cp < 0x80    && *len > 1) ||
        (cp < 0x800   && *len > 2) ||
        (cp < 0x10000 && *len > 3)) {
        return 0;
    }

    /* invalid unicode */
    if (cp > 0x10FFFF) {
        return 0;
    }

    /* surrogate halves */
    if (cp >= 0xD800 && cp <= 0xDFFF) {
        return 0;
    }

    return 1;
}

static int is_valid_utf8(const char *string, size_t string_len) {
    int len = 0;
    const char *string_end =  string + string_len;
    while (string < string_end) {
        if (!verify_utf8_sequence((const unsigned char*)string, &len)) {
            return 0;
        }
        string += len;
    }
    return 1;
}

static int is_decimal(const char *string, size_t length) {
    if (length > 1 && string[0] == '0' && string[1] != '.') {
        return 0;
    }
    if (length > 2 && !strncmp(string, "-0", 2) && string[2] != '.') {
        return 0;
    }
    while (length--) {
        if (strchr("xX", string[length])) {
            return 0;
        }
    }
    return 1;
}

static char * read_file(const char * filename) {
    FILE *fp = fopen(filename, "r");
    size_t file_size;
    long pos;
    char *file_contents;
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        return NULL;
    }
    file_size = pos;
    rewind(fp);
    file_contents = (char*)exparson_malloc(sizeof(char) * (file_size + 1));
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1) {
        if (ferror(fp)) {
            fclose(fp);
            exparson_free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    file_contents[file_size] = '\0';
    return file_contents;
}

static void remove_comments(char *string, const char *start_token, const char *end_token) {
    int in_string = 0, escaped = 0;
    size_t i;
    char *ptr = NULL, current_char;
    size_t start_token_len = strlen(start_token);
    size_t end_token_len = strlen(end_token);
    if (start_token_len == 0 || end_token_len == 0) {
        return;
    }
    while ((current_char = *string) != '\0') {
        if (current_char == '\\' && !escaped) {
            escaped = 1;
            string++;
            continue;
        } else if (current_char == '\"' && !escaped) {
            in_string = !in_string;
        } else if (!in_string && strncmp(string, start_token, start_token_len) == 0) {
            for(i = 0; i < start_token_len; i++) {
                string[i] = ' ';
            }
            string = string + start_token_len;
            ptr = strstr(string, end_token);
            if (!ptr) {
                return;
            }
            for (i = 0; i < (ptr - string) + end_token_len; i++) {
                string[i] = ' ';
            }
            string = ptr + end_token_len - 1;
        }
        escaped = 0;
        string++;
    }
}

/* EXJSON Object */
static EXJSON_Object * exjson_object_init(EXJSON_Value *wrapping_value) {
    EXJSON_Object *new_obj = (EXJSON_Object*)exparson_malloc(sizeof(EXJSON_Object));
    if (new_obj == NULL) {
        return NULL;
    }
    new_obj->wrapping_value = wrapping_value;
    new_obj->names = (char**)NULL;
    new_obj->values = (EXJSON_Value**)NULL;
    new_obj->capacity = 0;
    new_obj->count = 0;
    return new_obj;
}

static EXJSON_Status exjson_object_add(EXJSON_Object *object, const char *name, EXJSON_Value *value) {
    size_t index = 0;
    if (object == NULL || name == NULL || value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_object_get_value(object, name) != NULL) {
        return EXJSONFailure;
    }
    if (object->count >= object->capacity) {
        size_t new_capacity = MAX(object->capacity * 2, STARTING_CAPACITY);
        if (exjson_object_resize(object, new_capacity) == EXJSONFailure) {
            return EXJSONFailure;
        }
    }
    index = object->count;
    object->names[index] = exparson_strdup(name);
    if (object->names[index] == NULL) {
        return EXJSONFailure;
    }
    value->parent = exjson_object_get_wrapping_value(object);
    object->values[index] = value;
    object->count++;
    return EXJSONSuccess;
}

static EXJSON_Status exjson_object_resize(EXJSON_Object *object, size_t new_capacity) {
    char **temp_names = NULL;
    EXJSON_Value **temp_values = NULL;

    if ((object->names == NULL && object->values != NULL) ||
        (object->names != NULL && object->values == NULL) ||
        new_capacity == 0) {
            return EXJSONFailure; /* Shouldn't happen */
    }
    temp_names = (char**)exparson_malloc(new_capacity * sizeof(char*));
    if (temp_names == NULL) {
        return EXJSONFailure;
    }
    temp_values = (EXJSON_Value**)exparson_malloc(new_capacity * sizeof(EXJSON_Value*));
    if (temp_values == NULL) {
        exparson_free(temp_names);
        return EXJSONFailure;
    }
    if (object->names != NULL && object->values != NULL && object->count > 0) {
        memcpy(temp_names, object->names, object->count * sizeof(char*));
        memcpy(temp_values, object->values, object->count * sizeof(EXJSON_Value*));
    }
    exparson_free(object->names);
    exparson_free(object->values);
    object->names = temp_names;
    object->values = temp_values;
    object->capacity = new_capacity;
    return EXJSONSuccess;
}

static EXJSON_Value * exjson_object_nget_value(const EXJSON_Object *object, const char *name, size_t n) {
    size_t i, name_length;
    for (i = 0; i < exjson_object_get_count(object); i++) {
        name_length = strlen(object->names[i]);
        if (name_length != n) {
            continue;
        }
        if (strncmp(object->names[i], name, n) == 0) {
            return object->values[i];
        }
    }
    return NULL;
}

static void exjson_object_free(EXJSON_Object *object) {
    size_t i;
    for (i = 0; i < object->count; i++) {
        exparson_free(object->names[i]);
        exjson_value_free(object->values[i]);
    }
    exparson_free(object->names);
    exparson_free(object->values);
    exparson_free(object);
}

/* EXJSON Array */
static EXJSON_Array * exjson_array_init(EXJSON_Value *wrapping_value) {
    EXJSON_Array *new_array = (EXJSON_Array*)exparson_malloc(sizeof(EXJSON_Array));
    if (new_array == NULL) {
        return NULL;
    }
    new_array->wrapping_value = wrapping_value;
    new_array->items = (EXJSON_Value**)NULL;
    new_array->capacity = 0;
    new_array->count = 0;
    return new_array;
}

static EXJSON_Status exjson_array_add(EXJSON_Array *array, EXJSON_Value *value) {
    if (array->count >= array->capacity) {
        size_t new_capacity = MAX(array->capacity * 2, STARTING_CAPACITY);
        if (exjson_array_resize(array, new_capacity) == EXJSONFailure) {
            return EXJSONFailure;
        }
    }
    value->parent = exjson_array_get_wrapping_value(array);
    array->items[array->count] = value;
    array->count++;
    return EXJSONSuccess;
}

static EXJSON_Status exjson_array_resize(EXJSON_Array *array, size_t new_capacity) {
    EXJSON_Value **new_items = NULL;
    if (new_capacity == 0) {
        return EXJSONFailure;
    }
    new_items = (EXJSON_Value**)exparson_malloc(new_capacity * sizeof(EXJSON_Value*));
    if (new_items == NULL) {
        return EXJSONFailure;
    }
    if (array->items != NULL && array->count > 0) {
        memcpy(new_items, array->items, array->count * sizeof(EXJSON_Value*));
    }
    exparson_free(array->items);
    array->items = new_items;
    array->capacity = new_capacity;
    return EXJSONSuccess;
}

static void exjson_array_free(EXJSON_Array *array) {
    size_t i;
    for (i = 0; i < array->count; i++) {
        exjson_value_free(array->items[i]);
    }
    exparson_free(array->items);
    exparson_free(array);
}

/* EXJSON Value */
static EXJSON_Value * exjson_value_init_string_no_copy(char *string) {
    EXJSON_Value *new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONString;
    new_value->value.string = string;
    return new_value;
}

/* Parser */
static EXJSON_Status skip_quotes(const char **string) {
    if (**string != '\"') {
        return EXJSONFailure;
    }
    SKIP_CHAR(string);
    while (**string != '\"') {
        if (**string == '\0') {
            return EXJSONFailure;
        } else if (**string == '\\') {
            SKIP_CHAR(string);
            if (**string == '\0') {
                return EXJSONFailure;
            }
        }
        SKIP_CHAR(string);
    }
    SKIP_CHAR(string);
    return EXJSONSuccess;
}

static int parse_utf16(const char **unprocessed, char **processed) {
    unsigned int cp, lead, trail;
    int parse_succeeded = 0;
    char *processed_ptr = *processed;
    const char *unprocessed_ptr = *unprocessed;
    unprocessed_ptr++; /* skips u */
    parse_succeeded = parse_utf16_hex(unprocessed_ptr, &cp);
    if (!parse_succeeded) {
        return EXJSONFailure;
    }
    if (cp < 0x80) {
        processed_ptr[0] = (char)cp; /* 0xxxxxxx */
    } else if (cp < 0x800) {
        processed_ptr[0] = ((cp >> 6) & 0x1F) | 0xC0; /* 110xxxxx */
        processed_ptr[1] = ((cp)      & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr += 1;
    } else if (cp < 0xD800 || cp > 0xDFFF) {
        processed_ptr[0] = ((cp >> 12) & 0x0F) | 0xE0; /* 1110xxxx */
        processed_ptr[1] = ((cp >> 6)  & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr[2] = ((cp)       & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr += 2;
    } else if (cp >= 0xD800 && cp <= 0xDBFF) { /* lead surrogate (0xD800..0xDBFF) */
        lead = cp;
        unprocessed_ptr += 4; /* should always be within the buffer, otherwise previous sscanf would fail */
        if (*unprocessed_ptr++ != '\\' || *unprocessed_ptr++ != 'u') {
            return EXJSONFailure;
        }
        parse_succeeded = parse_utf16_hex(unprocessed_ptr, &trail);
        if (!parse_succeeded || trail < 0xDC00 || trail > 0xDFFF) { /* valid trail surrogate? (0xDC00..0xDFFF) */
            return EXJSONFailure;
        }
        cp = ((((lead - 0xD800) & 0x3FF) << 10) | ((trail - 0xDC00) & 0x3FF)) + 0x010000;
        processed_ptr[0] = (((cp >> 18) & 0x07) | 0xF0); /* 11110xxx */
        processed_ptr[1] = (((cp >> 12) & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr[2] = (((cp >> 6)  & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr[3] = (((cp)       & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr += 3;
    } else { /* trail surrogate before lead surrogate */
        return EXJSONFailure;
    }
    unprocessed_ptr += 3;
    *processed = processed_ptr;
    *unprocessed = unprocessed_ptr;
    return EXJSONSuccess;
}


/* Copies and processes passed string up to supplied length.
Example: "\u006Corem ipsum" -> lorem ipsum */
static char* process_string(const char *input, size_t len) {
    const char *input_ptr = input;
    size_t initial_size = (len + 1) * sizeof(char);
    size_t final_size = 0;
    char *output = NULL, *output_ptr = NULL, *resized_output = NULL;
    output = (char*)exparson_malloc(initial_size);
    if (output == NULL) {
        goto error;
    }
    output_ptr = output;
    while ((*input_ptr != '\0') && (size_t)(input_ptr - input) < len) {
        if (*input_ptr == '\\') {
            input_ptr++;
            switch (*input_ptr) {
                case '\"': *output_ptr = '\"'; break;
                case '\\': *output_ptr = '\\'; break;
                case '/':  *output_ptr = '/';  break;
                case 'b':  *output_ptr = '\b'; break;
                case 'f':  *output_ptr = '\f'; break;
                case 'n':  *output_ptr = '\n'; break;
                case 'r':  *output_ptr = '\r'; break;
                case 't':  *output_ptr = '\t'; break;
                case 'u':
                    if (parse_utf16(&input_ptr, &output_ptr) == EXJSONFailure) {
                        goto error;
                    }
                    break;
                default:
                    goto error;
            }
        } else if ((unsigned char)*input_ptr < 0x20) {
            goto error; /* 0x00-0x19 are invalid characters for exjson string (http://www.ietf.org/rfc/rfc4627.txt) */
        } else {
            *output_ptr = *input_ptr;
        }
        output_ptr++;
        input_ptr++;
    }
    *output_ptr = '\0';
    /* resize to new length */
    final_size = (size_t)(output_ptr-output) + 1;
    /* todo: don't resize if final_size == initial_size */
    resized_output = (char*)exparson_malloc(final_size);
    if (resized_output == NULL) {
        goto error;
    }
    memcpy(resized_output, output, final_size);
    exparson_free(output);
    return resized_output;
error:
    exparson_free(output);
    return NULL;
}

/* Return processed contents of a string between quotes and
   skips passed argument to a matching quote. */
static char * get_quoted_string(const char **string) {
    const char *string_start = *string;
    size_t string_len = 0;
    EXJSON_Status status = skip_quotes(string);
    if (status != EXJSONSuccess) {
        return NULL;
    }
    string_len = *string - string_start - 2; /* length without quotes */
    return process_string(string_start + 1, string_len);
}

static EXJSON_Value * parse_value(const char **string, size_t nesting) {
    if (nesting > MAX_NESTING) {
        return NULL;
    }
    SKIP_WHITESPACES(string);
    switch (**string) {
        case '{':
            return parse_object_value(string, nesting + 1);
        case '[':
            return parse_array_value(string, nesting + 1);
        case '\"':
            return parse_string_value(string);
        case 'f': case 't':
            return parse_boolean_value(string);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number_value(string);
        case 'n':
            return parse_null_value(string);
        default:
            return NULL;
    }
}

static EXJSON_Value * parse_object_value(const char **string, size_t nesting) {
    EXJSON_Value *output_value = exjson_value_init_object(), *new_value = NULL;
    EXJSON_Object *output_object = exjson_value_get_object(output_value);
    char *new_key = NULL;
    if (output_value == NULL || **string != '{') {
        return NULL;
    }
    SKIP_CHAR(string);
    SKIP_WHITESPACES(string);
    if (**string == '}') { /* empty object */
        SKIP_CHAR(string);
        return output_value;
    }
    while (**string != '\0') {
        new_key = get_quoted_string(string);
        if (new_key == NULL) {
            exjson_value_free(output_value);
            return NULL;
        }
        SKIP_WHITESPACES(string);
        if (**string != ':') {
            exparson_free(new_key);
            exjson_value_free(output_value);
            return NULL;
        }
        SKIP_CHAR(string);
        new_value = parse_value(string, nesting);
        if (new_value == NULL) {
            exparson_free(new_key);
            exjson_value_free(output_value);
            return NULL;
        }
        if (exjson_object_add(output_object, new_key, new_value) == EXJSONFailure) {
            exparson_free(new_key);
            exjson_value_free(new_value);
            exjson_value_free(output_value);
            return NULL;
        }
        exparson_free(new_key);
        SKIP_WHITESPACES(string);
        if (**string != ',') {
            break;
        }
        SKIP_CHAR(string);
        SKIP_WHITESPACES(string);
    }
    SKIP_WHITESPACES(string);
    if (**string != '}' || /* Trim object after parsing is over */
        exjson_object_resize(output_object, exjson_object_get_count(output_object)) == EXJSONFailure) {
            exjson_value_free(output_value);
            return NULL;
    }
    SKIP_CHAR(string);
    return output_value;
}

static EXJSON_Value * parse_array_value(const char **string, size_t nesting) {
    EXJSON_Value *output_value = exjson_value_init_array(), *new_array_value = NULL;
    EXJSON_Array *output_array = exjson_value_get_array(output_value);
    if (!output_value || **string != '[') {
        return NULL;
    }
    SKIP_CHAR(string);
    SKIP_WHITESPACES(string);
    if (**string == ']') { /* empty array */
        SKIP_CHAR(string);
        return output_value;
    }
    while (**string != '\0') {
        new_array_value = parse_value(string, nesting);
        if (new_array_value == NULL) {
            exjson_value_free(output_value);
            return NULL;
        }
        if (exjson_array_add(output_array, new_array_value) == EXJSONFailure) {
            exjson_value_free(new_array_value);
            exjson_value_free(output_value);
            return NULL;
        }
        SKIP_WHITESPACES(string);
        if (**string != ',') {
            break;
        }
        SKIP_CHAR(string);
        SKIP_WHITESPACES(string);
    }
    SKIP_WHITESPACES(string);
    if (**string != ']' || /* Trim array after parsing is over */
        exjson_array_resize(output_array, exjson_array_get_count(output_array)) == EXJSONFailure) {
            exjson_value_free(output_value);
            return NULL;
    }
    SKIP_CHAR(string);
    return output_value;
}

static EXJSON_Value * parse_string_value(const char **string) {
    EXJSON_Value *value = NULL;
    char *new_string = get_quoted_string(string);
    if (new_string == NULL) {
        return NULL;
    }
    value = exjson_value_init_string_no_copy(new_string);
    if (value == NULL) {
        exparson_free(new_string);
        return NULL;
    }
    return value;
}

static EXJSON_Value * parse_boolean_value(const char **string) {
    size_t true_token_size = SIZEOF_TOKEN("true");
    size_t false_token_size = SIZEOF_TOKEN("false");
    if (strncmp("true", *string, true_token_size) == 0) {
        *string += true_token_size;
        return exjson_value_init_boolean(1);
    } else if (strncmp("false", *string, false_token_size) == 0) {
        *string += false_token_size;
        return exjson_value_init_boolean(0);
    }
    return NULL;
}

static EXJSON_Value * parse_number_value(const char **string) {
    char *end;
    double number = 0;
    errno = 0;
    number = strtod(*string, &end);
    if (errno || !is_decimal(*string, end - *string)) {
        return NULL;
    }
    *string = end;
    return exjson_value_init_number(number);
}

static EXJSON_Value * parse_null_value(const char **string) {
    size_t token_size = SIZEOF_TOKEN("null");
    if (strncmp("null", *string, token_size) == 0) {
        *string += token_size;
        return exjson_value_init_null();
    }
    return NULL;
}

/* Serialization */
#define APPEND_STRING(str) do { written = append_string(buf, (str));\
                                if (written < 0) { return -1; }\
                                if (buf != NULL) { buf += written; }\
                                written_total += written; } while(0)

#define APPEND_INDENT(level) do { written = append_indent(buf, (level));\
                                  if (written < 0) { return -1; }\
                                  if (buf != NULL) { buf += written; }\
                                  written_total += written; } while(0)

static int exjson_serialize_to_buffer_r(const EXJSON_Value *value, char *buf, int level, int is_pretty, char *num_buf)
{
    const char *key = NULL, *string = NULL;
    EXJSON_Value *temp_value = NULL;
    EXJSON_Array *array = NULL;
    EXJSON_Object *object = NULL;
    size_t i = 0, count = 0;
    double num = 0.0;
    int written = -1, written_total = 0;

    switch (exjson_value_get_type(value)) {
        case EXJSONArray:
            array = exjson_value_get_array(value);
            count = exjson_array_get_count(array);
            APPEND_STRING("[");
            if (count > 0 && is_pretty) {
                APPEND_STRING("\n");
            }
            for (i = 0; i < count; i++) {
                if (is_pretty) {
                    APPEND_INDENT(level+1);
                }
                temp_value = exjson_array_get_value(array, i);
                written = exjson_serialize_to_buffer_r(temp_value, buf, level+1, is_pretty, num_buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                if (i < (count - 1)) {
                    APPEND_STRING(",");
                }
                if (is_pretty) {
                    APPEND_STRING("\n");
                }
            }
            if (count > 0 && is_pretty) {
                APPEND_INDENT(level);
            }
            APPEND_STRING("]");
            return written_total;
        case EXJSONObject:
            object = exjson_value_get_object(value);
            count  = exjson_object_get_count(object);
            APPEND_STRING("{");
            if (count > 0 && is_pretty) {
                APPEND_STRING("\n");
            }
            for (i = 0; i < count; i++) {
                key = exjson_object_get_name(object, i);
                if (key == NULL) {
                    return -1;
                }
                if (is_pretty) {
                    APPEND_INDENT(level+1);
                }
                written = exjson_serialize_string(key, buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                APPEND_STRING(":");
                if (is_pretty) {
                    APPEND_STRING(" ");
                }
                temp_value = exjson_object_get_value(object, key);
                written = exjson_serialize_to_buffer_r(temp_value, buf, level+1, is_pretty, num_buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                if (i < (count - 1)) {
                    APPEND_STRING(",");
                }
                if (is_pretty) {
                    APPEND_STRING("\n");
                }
            }
            if (count > 0 && is_pretty) {
                APPEND_INDENT(level);
            }
            APPEND_STRING("}");
            return written_total;
        case EXJSONString:
            string = exjson_value_get_string(value);
            if (string == NULL) {
                return -1;
            }
            written = exjson_serialize_string(string, buf);
            if (written < 0) {
                return -1;
            }
            if (buf != NULL) {
                buf += written;
            }
            written_total += written;
            return written_total;
        case EXJSONBoolean:
            if (exjson_value_get_boolean(value)) {
                APPEND_STRING("true");
            } else {
                APPEND_STRING("false");
            }
            return written_total;
        case EXJSONNumber:
            num = exjson_value_get_number(value);
            if (buf != NULL) {
                num_buf = buf;
            }
            written = sprintf(num_buf, FLOAT_FORMAT, num);
            if (written < 0) {
                return -1;
            }
            if (buf != NULL) {
                buf += written;
            }
            written_total += written;
            return written_total;
        case EXJSONNull:
            APPEND_STRING("null");
            return written_total;
        case EXJSONError:
            return -1;
        default:
            return -1;
    }
}

static int exjson_serialize_string(const char *string, char *buf) {
    size_t i = 0, len = strlen(string);
    char c = '\0';
    int written = -1, written_total = 0;
    APPEND_STRING("\"");
    for (i = 0; i < len; i++) {
        c = string[i];
        switch (c) {
            case '\"': APPEND_STRING("\\\""); break;
            case '\\': APPEND_STRING("\\\\"); break;
            case '/':  APPEND_STRING("\\/"); break; /* to make exjson embeddable in xml\/html */
            case '\b': APPEND_STRING("\\b"); break;
            case '\f': APPEND_STRING("\\f"); break;
            case '\n': APPEND_STRING("\\n"); break;
            case '\r': APPEND_STRING("\\r"); break;
            case '\t': APPEND_STRING("\\t"); break;
            case '\x00': APPEND_STRING("\\u0000"); break;
            case '\x01': APPEND_STRING("\\u0001"); break;
            case '\x02': APPEND_STRING("\\u0002"); break;
            case '\x03': APPEND_STRING("\\u0003"); break;
            case '\x04': APPEND_STRING("\\u0004"); break;
            case '\x05': APPEND_STRING("\\u0005"); break;
            case '\x06': APPEND_STRING("\\u0006"); break;
            case '\x07': APPEND_STRING("\\u0007"); break;
            /* '\x08' duplicate: '\b' */
            /* '\x09' duplicate: '\t' */
            /* '\x0a' duplicate: '\n' */
            case '\x0b': APPEND_STRING("\\u000b"); break;
            /* '\x0c' duplicate: '\f' */
            /* '\x0d' duplicate: '\r' */
            case '\x0e': APPEND_STRING("\\u000e"); break;
            case '\x0f': APPEND_STRING("\\u000f"); break;
            case '\x10': APPEND_STRING("\\u0010"); break;
            case '\x11': APPEND_STRING("\\u0011"); break;
            case '\x12': APPEND_STRING("\\u0012"); break;
            case '\x13': APPEND_STRING("\\u0013"); break;
            case '\x14': APPEND_STRING("\\u0014"); break;
            case '\x15': APPEND_STRING("\\u0015"); break;
            case '\x16': APPEND_STRING("\\u0016"); break;
            case '\x17': APPEND_STRING("\\u0017"); break;
            case '\x18': APPEND_STRING("\\u0018"); break;
            case '\x19': APPEND_STRING("\\u0019"); break;
            case '\x1a': APPEND_STRING("\\u001a"); break;
            case '\x1b': APPEND_STRING("\\u001b"); break;
            case '\x1c': APPEND_STRING("\\u001c"); break;
            case '\x1d': APPEND_STRING("\\u001d"); break;
            case '\x1e': APPEND_STRING("\\u001e"); break;
            case '\x1f': APPEND_STRING("\\u001f"); break;
            default:
                if (buf != NULL) {
                    buf[0] = c;
                    buf += 1;
                }
                written_total += 1;
                break;
        }
    }
    APPEND_STRING("\"");
    return written_total;
}

static int append_indent(char *buf, int level) {
    int i;
    int written = -1, written_total = 0;
    for (i = 0; i < level; i++) {
        APPEND_STRING("    ");
    }
    return written_total;
}

static int append_string(char *buf, const char *string) {
    if (buf == NULL) {
        return (int)strlen(string);
    }
    return sprintf(buf, "%s", string);
}

#undef APPEND_STRING
#undef APPEND_INDENT

/* Parser API */
EXJSON_Value * exjson_parse_file(const char *filename) {
    char *file_contents = read_file(filename);
    EXJSON_Value *output_value = NULL;
    if (file_contents == NULL) {
        return NULL;
    }
    output_value = exjson_parse_string(file_contents);
    exparson_free(file_contents);
    return output_value;
}

EXJSON_Value * exjson_parse_file_with_comments(const char *filename) {
    char *file_contents = read_file(filename);
    EXJSON_Value *output_value = NULL;
    if (file_contents == NULL) {
        return NULL;
    }
    output_value = exjson_parse_string_with_comments(file_contents);
    exparson_free(file_contents);
    return output_value;
}

EXJSON_Value * exjson_parse_string(const char *string) {
    if (string == NULL) {
        return NULL;
    }
    if (string[0] == '\xEF' && string[1] == '\xBB' && string[2] == '\xBF') {
        string = string + 3; /* Support for UTF-8 BOM */
    }
    return parse_value((const char**)&string, 0);
}

EXJSON_Value * exjson_parse_string_with_comments(const char *string) {
    EXJSON_Value *result = NULL;
    char *string_mutable_copy = NULL, *string_mutable_copy_ptr = NULL;
    string_mutable_copy = exparson_strdup(string);
    if (string_mutable_copy == NULL) {
        return NULL;
    }
    remove_comments(string_mutable_copy, "/*", "*/");
    remove_comments(string_mutable_copy, "//", "\n");
    string_mutable_copy_ptr = string_mutable_copy;
    result = parse_value((const char**)&string_mutable_copy_ptr, 0);
    exparson_free(string_mutable_copy);
    return result;
}

/* EXJSON Object API */

EXJSON_Value * exjson_object_get_value(const EXJSON_Object *object, const char *name) {
    if (object == NULL || name == NULL) {
        return NULL;
    }
    return exjson_object_nget_value(object, name, strlen(name));
}

const char * exjson_object_get_string(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_string(exjson_object_get_value(object, name));
}

double exjson_object_get_number(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_number(exjson_object_get_value(object, name));
}

EXJSON_Object * exjson_object_get_object(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_object(exjson_object_get_value(object, name));
}

EXJSON_Array * exjson_object_get_array(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_array(exjson_object_get_value(object, name));
}

int exjson_object_get_boolean(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_boolean(exjson_object_get_value(object, name));
}

EXJSON_Value * exjson_object_dotget_value(const EXJSON_Object *object, const char *name) {
    const char *dot_position = strchr(name, '.');
    if (!dot_position) {
        return exjson_object_get_value(object, name);
    }
    object = exjson_value_get_object(exjson_object_nget_value(object, name, dot_position - name));
    return exjson_object_dotget_value(object, dot_position + 1);
}

const char * exjson_object_dotget_string(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_string(exjson_object_dotget_value(object, name));
}

double exjson_object_dotget_number(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_number(exjson_object_dotget_value(object, name));
}

EXJSON_Object * exjson_object_dotget_object(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_object(exjson_object_dotget_value(object, name));
}

EXJSON_Array * exjson_object_dotget_array(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_array(exjson_object_dotget_value(object, name));
}

int exjson_object_dotget_boolean(const EXJSON_Object *object, const char *name) {
    return exjson_value_get_boolean(exjson_object_dotget_value(object, name));
}

size_t exjson_object_get_count(const EXJSON_Object *object) {
    return object ? object->count : 0;
}

const char * exjson_object_get_name(const EXJSON_Object *object, size_t index) {
    if (object == NULL || index >= exjson_object_get_count(object)) {
        return NULL;
    }
    return object->names[index];
}

EXJSON_Value * exjson_object_get_value_at(const EXJSON_Object *object, size_t index) {
    if (object == NULL || index >= exjson_object_get_count(object)) {
        return NULL;
    }
    return object->values[index];
}

EXJSON_Value *exjson_object_get_wrapping_value(const EXJSON_Object *object) {
    return object->wrapping_value;
}

int exjson_object_has_value (const EXJSON_Object *object, const char *name) {
    return exjson_object_get_value(object, name) != NULL;
}

int exjson_object_has_value_of_type(const EXJSON_Object *object, const char *name, EXJSON_Value_Type type) {
    EXJSON_Value *val = exjson_object_get_value(object, name);
    return val != NULL && exjson_value_get_type(val) == type;
}

int exjson_object_dothas_value (const EXJSON_Object *object, const char *name) {
    return exjson_object_dotget_value(object, name) != NULL;
}

int exjson_object_dothas_value_of_type(const EXJSON_Object *object, const char *name, EXJSON_Value_Type type) {
    EXJSON_Value *val = exjson_object_dotget_value(object, name);
    return val != NULL && exjson_value_get_type(val) == type;
}

/* EXJSON Array API */
EXJSON_Value * exjson_array_get_value(const EXJSON_Array *array, size_t index) {
    if (array == NULL || index >= exjson_array_get_count(array)) {
        return NULL;
    }
    return array->items[index];
}

const char * exjson_array_get_string(const EXJSON_Array *array, size_t index) {
    return exjson_value_get_string(exjson_array_get_value(array, index));
}

double exjson_array_get_number(const EXJSON_Array *array, size_t index) {
    return exjson_value_get_number(exjson_array_get_value(array, index));
}

EXJSON_Object * exjson_array_get_object(const EXJSON_Array *array, size_t index) {
    return exjson_value_get_object(exjson_array_get_value(array, index));
}

EXJSON_Array * exjson_array_get_array(const EXJSON_Array *array, size_t index) {
    return exjson_value_get_array(exjson_array_get_value(array, index));
}

int exjson_array_get_boolean(const EXJSON_Array *array, size_t index) {
    return exjson_value_get_boolean(exjson_array_get_value(array, index));
}

size_t exjson_array_get_count(const EXJSON_Array *array) {
    return array ? array->count : 0;
}

EXJSON_Value * exjson_array_get_wrapping_value(const EXJSON_Array *array) {
    return array->wrapping_value;
}

/* EXJSON Value API */
EXJSON_Value_Type exjson_value_get_type(const EXJSON_Value *value) {
    return value ? value->type : EXJSONError;
}

EXJSON_Object * exjson_value_get_object(const EXJSON_Value *value) {
    return exjson_value_get_type(value) == EXJSONObject ? value->value.object : NULL;
}

EXJSON_Array * exjson_value_get_array(const EXJSON_Value *value) {
    return exjson_value_get_type(value) == EXJSONArray ? value->value.array : NULL;
}

const char * exjson_value_get_string(const EXJSON_Value *value) {
    return exjson_value_get_type(value) == EXJSONString ? value->value.string : NULL;
}

double exjson_value_get_number(const EXJSON_Value *value) {
    return exjson_value_get_type(value) == EXJSONNumber ? value->value.number : 0;
}

int exjson_value_get_boolean(const EXJSON_Value *value) {
    return exjson_value_get_type(value) == EXJSONBoolean ? value->value.boolean : -1;
}

EXJSON_Value * exjson_value_get_parent (const EXJSON_Value *value) {
    return value ? value->parent : NULL;
}

void exjson_value_free(EXJSON_Value *value) {
    switch (exjson_value_get_type(value)) {
        case EXJSONObject:
            exjson_object_free(value->value.object);
            break;
        case EXJSONString:
            exparson_free(value->value.string);
            break;
        case EXJSONArray:
            exjson_array_free(value->value.array);
            break;
        default:
            break;
    }
    exparson_free(value);
}

EXJSON_Value * exjson_value_init_object(void) {
    EXJSON_Value *new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONObject;
    new_value->value.object = exjson_object_init(new_value);
    if (!new_value->value.object) {
        exparson_free(new_value);
        return NULL;
    }
    return new_value;
}

EXJSON_Value * exjson_value_init_array(void) {
    EXJSON_Value *new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONArray;
    new_value->value.array = exjson_array_init(new_value);
    if (!new_value->value.array) {
        exparson_free(new_value);
        return NULL;
    }
    return new_value;
}

EXJSON_Value * exjson_value_init_string(const char *string) {
    char *copy = NULL;
    EXJSON_Value *value;
    size_t string_len = 0;
    if (string == NULL) {
        return NULL;
    }
    string_len = strlen(string);
    if (!is_valid_utf8(string, string_len)) {
        return NULL;
    }
    copy = exparson_strndup(string, string_len);
    if (copy == NULL) {
        return NULL;
    }
    value = exjson_value_init_string_no_copy(copy);
    if (value == NULL) {
        exparson_free(copy);
    }
    return value;
}

EXJSON_Value * exjson_value_init_number(double number) {
    EXJSON_Value *new_value = NULL;
    if ((number * 0.0) != 0.0) { /* nan and inf test */
        return NULL;
    }
    new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (new_value == NULL) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONNumber;
    new_value->value.number = number;
    return new_value;
}

EXJSON_Value * exjson_value_init_boolean(int boolean) {
    EXJSON_Value *new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONBoolean;
    new_value->value.boolean = boolean ? 1 : 0;
    return new_value;
}

EXJSON_Value * exjson_value_init_null(void) {
    EXJSON_Value *new_value = (EXJSON_Value*)exparson_malloc(sizeof(EXJSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = EXJSONNull;
    return new_value;
}

EXJSON_Value * exjson_value_deep_copy(const EXJSON_Value *value) {
    size_t i = 0;
    EXJSON_Value *return_value = NULL, *temp_value_copy = NULL, *temp_value = NULL;
    const char *temp_string = NULL, *temp_key = NULL;
    char *temp_string_copy = NULL;
    EXJSON_Array *temp_array = NULL, *temp_array_copy = NULL;
    EXJSON_Object *temp_object = NULL, *temp_object_copy = NULL;

    switch (exjson_value_get_type(value)) {
        case EXJSONArray:
            temp_array = exjson_value_get_array(value);
            return_value = exjson_value_init_array();
            if (return_value == NULL) {
                return NULL;
            }
            temp_array_copy = exjson_value_get_array(return_value);
            for (i = 0; i < exjson_array_get_count(temp_array); i++) {
                temp_value = exjson_array_get_value(temp_array, i);
                temp_value_copy = exjson_value_deep_copy(temp_value);
                if (temp_value_copy == NULL) {
                    exjson_value_free(return_value);
                    return NULL;
                }
                if (exjson_array_add(temp_array_copy, temp_value_copy) == EXJSONFailure) {
                    exjson_value_free(return_value);
                    exjson_value_free(temp_value_copy);
                    return NULL;
                }
            }
            return return_value;
        case EXJSONObject:
            temp_object = exjson_value_get_object(value);
            return_value = exjson_value_init_object();
            if (return_value == NULL) {
                return NULL;
            }
            temp_object_copy = exjson_value_get_object(return_value);
            for (i = 0; i < exjson_object_get_count(temp_object); i++) {
                temp_key = exjson_object_get_name(temp_object, i);
                temp_value = exjson_object_get_value(temp_object, temp_key);
                temp_value_copy = exjson_value_deep_copy(temp_value);
                if (temp_value_copy == NULL) {
                    exjson_value_free(return_value);
                    return NULL;
                }
                if (exjson_object_add(temp_object_copy, temp_key, temp_value_copy) == EXJSONFailure) {
                    exjson_value_free(return_value);
                    exjson_value_free(temp_value_copy);
                    return NULL;
                }
            }
            return return_value;
        case EXJSONBoolean:
            return exjson_value_init_boolean(exjson_value_get_boolean(value));
        case EXJSONNumber:
            return exjson_value_init_number(exjson_value_get_number(value));
        case EXJSONString:
            temp_string = exjson_value_get_string(value);
            if (temp_string == NULL) {
                return NULL;
            }
            temp_string_copy = exparson_strdup(temp_string);
            if (temp_string_copy == NULL) {
                return NULL;
            }
            return_value = exjson_value_init_string_no_copy(temp_string_copy);
            if (return_value == NULL) {
                exparson_free(temp_string_copy);
            }
            return return_value;
        case EXJSONNull:
            return exjson_value_init_null();
        case EXJSONError:
            return NULL;
        default:
            return NULL;
    }
}

size_t exjson_serialization_size(const EXJSON_Value *value) {
    char num_buf[1100]; /* recursively allocating buffer on stack is a bad idea, so let's do it only once */
    int res = exjson_serialize_to_buffer_r(value, NULL, 0, 0, num_buf);
    return res < 0 ? 0 : (size_t)(res + 1);
}

EXJSON_Status exjson_serialize_to_buffer(const EXJSON_Value *value, char *buf, size_t buf_size_in_bytes) {
    int written = -1;
    size_t needed_size_in_bytes = exjson_serialization_size(value);
    if (needed_size_in_bytes == 0 || buf_size_in_bytes < needed_size_in_bytes) {
        return EXJSONFailure;
    }
    written = exjson_serialize_to_buffer_r(value, buf, 0, 0, NULL);
    if (written < 0) {
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_serialize_to_file(const EXJSON_Value *value, const char *filename) {
    EXJSON_Status return_code = EXJSONSuccess;
    FILE *fp = NULL;
    char *serialized_string = exjson_serialize_to_string(value);
    if (serialized_string == NULL) {
        return EXJSONFailure;
    }
    fp = fopen (filename, "w");
    if (fp == NULL) {
        exjson_free_serialized_string(serialized_string);
        return EXJSONFailure;
    }
    if (fputs(serialized_string, fp) == EOF) {
        return_code = EXJSONFailure;
    }
    if (fclose(fp) == EOF) {
        return_code = EXJSONFailure;
    }
    exjson_free_serialized_string(serialized_string);
    return return_code;
}

char * exjson_serialize_to_string(const EXJSON_Value *value) {
    EXJSON_Status serialization_result = EXJSONFailure;
    size_t buf_size_bytes = exjson_serialization_size(value);
    char *buf = NULL;
    if (buf_size_bytes == 0) {
        return NULL;
    }
    buf = (char*)exparson_malloc(buf_size_bytes);
    if (buf == NULL) {
        return NULL;
    }
    serialization_result = exjson_serialize_to_buffer(value, buf, buf_size_bytes);
    if (serialization_result == EXJSONFailure) {
        exjson_free_serialized_string(buf);
        return NULL;
    }
    return buf;
}

size_t exjson_serialization_size_pretty(const EXJSON_Value *value) {
    char num_buf[1100]; /* recursively allocating buffer on stack is a bad idea, so let's do it only once */
    int res = exjson_serialize_to_buffer_r(value, NULL, 0, 1, num_buf);
    return res < 0 ? 0 : (size_t)(res + 1);
}

EXJSON_Status exjson_serialize_to_buffer_pretty(const EXJSON_Value *value, char *buf, size_t buf_size_in_bytes) {
    int written = -1;
    size_t needed_size_in_bytes = exjson_serialization_size_pretty(value);
    if (needed_size_in_bytes == 0 || buf_size_in_bytes < needed_size_in_bytes) {
        return EXJSONFailure;
    }
    written = exjson_serialize_to_buffer_r(value, buf, 0, 1, NULL);
    if (written < 0) {
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_serialize_to_file_pretty(const EXJSON_Value *value, const char *filename) {
    EXJSON_Status return_code = EXJSONSuccess;
    FILE *fp = NULL;
    char *serialized_string = exjson_serialize_to_string_pretty(value);
    if (serialized_string == NULL) {
        return EXJSONFailure;
    }
    fp = fopen (filename, "w");
    if (fp == NULL) {
        exjson_free_serialized_string(serialized_string);
        return EXJSONFailure;
    }
    if (fputs(serialized_string, fp) == EOF) {
        return_code = EXJSONFailure;
    }
    if (fclose(fp) == EOF) {
        return_code = EXJSONFailure;
    }
    exjson_free_serialized_string(serialized_string);
    return return_code;
}

char * exjson_serialize_to_string_pretty(const EXJSON_Value *value) {
    EXJSON_Status serialization_result = EXJSONFailure;
    size_t buf_size_bytes = exjson_serialization_size_pretty(value);
    char *buf = NULL;
    if (buf_size_bytes == 0) {
        return NULL;
    }
    buf = (char*)exparson_malloc(buf_size_bytes);
    if (buf == NULL) {
        return NULL;
    }
    serialization_result = exjson_serialize_to_buffer_pretty(value, buf, buf_size_bytes);
    if (serialization_result == EXJSONFailure) {
        exjson_free_serialized_string(buf);
        return NULL;
    }
    return buf;
}

void exjson_free_serialized_string(char *string) {
    exparson_free(string);
}

EXJSON_Status exjson_array_remove(EXJSON_Array *array, size_t ix) {
    size_t to_move_bytes = 0;
    if (array == NULL || ix >= exjson_array_get_count(array)) {
        return EXJSONFailure;
    }
    exjson_value_free(exjson_array_get_value(array, ix));
    to_move_bytes = (exjson_array_get_count(array) - 1 - ix) * sizeof(EXJSON_Value*);
    memmove(array->items + ix, array->items + ix + 1, to_move_bytes);
    array->count -= 1;
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_replace_value(EXJSON_Array *array, size_t ix, EXJSON_Value *value) {
    if (array == NULL || value == NULL || value->parent != NULL || ix >= exjson_array_get_count(array)) {
        return EXJSONFailure;
    }
    exjson_value_free(exjson_array_get_value(array, ix));
    value->parent = exjson_array_get_wrapping_value(array);
    array->items[ix] = value;
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_replace_string(EXJSON_Array *array, size_t i, const char* string) {
    EXJSON_Value *value = exjson_value_init_string(string);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_replace_value(array, i, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_replace_number(EXJSON_Array *array, size_t i, double number) {
    EXJSON_Value *value = exjson_value_init_number(number);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_replace_value(array, i, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_replace_boolean(EXJSON_Array *array, size_t i, int boolean) {
    EXJSON_Value *value = exjson_value_init_boolean(boolean);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_replace_value(array, i, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_replace_null(EXJSON_Array *array, size_t i) {
    EXJSON_Value *value = exjson_value_init_null();
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_replace_value(array, i, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_clear(EXJSON_Array *array) {
    size_t i = 0;
    if (array == NULL) {
        return EXJSONFailure;
    }
    for (i = 0; i < exjson_array_get_count(array); i++) {
        exjson_value_free(exjson_array_get_value(array, i));
    }
    array->count = 0;
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_append_value(EXJSON_Array *array, EXJSON_Value *value) {
    if (array == NULL || value == NULL || value->parent != NULL) {
        return EXJSONFailure;
    }
    return exjson_array_add(array, value);
}

EXJSON_Status exjson_array_append_string(EXJSON_Array *array, const char *string) {
    EXJSON_Value *value = exjson_value_init_string(string);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_append_value(array, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_append_number(EXJSON_Array *array, double number) {
    EXJSON_Value *value = exjson_value_init_number(number);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_append_value(array, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_append_boolean(EXJSON_Array *array, int boolean) {
    EXJSON_Value *value = exjson_value_init_boolean(boolean);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_append_value(array, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_array_append_null(EXJSON_Array *array) {
    EXJSON_Value *value = exjson_value_init_null();
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_array_append_value(array, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_object_set_value(EXJSON_Object *object, const char *name, EXJSON_Value *value) {
    size_t i = 0;
    EXJSON_Value *old_value;
    if (object == NULL || name == NULL || value == NULL || value->parent != NULL) {
        return EXJSONFailure;
    }
    old_value = exjson_object_get_value(object, name);
    if (old_value != NULL) { /* free and overwrite old value */
        exjson_value_free(old_value);
        for (i = 0; i < exjson_object_get_count(object); i++) {
            if (strcmp(object->names[i], name) == 0) {
                value->parent = exjson_object_get_wrapping_value(object);
                object->values[i] = value;
                return EXJSONSuccess;
            }
        }
    }
    /* add new key value pair */
    return exjson_object_add(object, name, value);
}

EXJSON_Status exjson_object_set_string(EXJSON_Object *object, const char *name, const char *string) {
    return exjson_object_set_value(object, name, exjson_value_init_string(string));
}

EXJSON_Status exjson_object_set_number(EXJSON_Object *object, const char *name, double number) {
    return exjson_object_set_value(object, name, exjson_value_init_number(number));
}

EXJSON_Status exjson_object_set_boolean(EXJSON_Object *object, const char *name, int boolean) {
    return exjson_object_set_value(object, name, exjson_value_init_boolean(boolean));
}

EXJSON_Status exjson_object_set_null(EXJSON_Object *object, const char *name) {
    return exjson_object_set_value(object, name, exjson_value_init_null());
}

EXJSON_Status exjson_object_dotset_value(EXJSON_Object *object, const char *name, EXJSON_Value *value) {
    const char *dot_pos = NULL;
    char *current_name = NULL;
    EXJSON_Object *temp_obj = NULL;
    EXJSON_Value *new_value = NULL;
    if (object == NULL || name == NULL || value == NULL) {
        return EXJSONFailure;
    }
    dot_pos = strchr(name, '.');
    if (dot_pos == NULL) {
        return exjson_object_set_value(object, name, value);
    } else {
        current_name = exparson_strndup(name, dot_pos - name);
        temp_obj = exjson_object_get_object(object, current_name);
        if (temp_obj == NULL) {
            new_value = exjson_value_init_object();
            if (new_value == NULL) {
                exparson_free(current_name);
                return EXJSONFailure;
            }
            if (exjson_object_add(object, current_name, new_value) == EXJSONFailure) {
                exjson_value_free(new_value);
                exparson_free(current_name);
                return EXJSONFailure;
            }
            temp_obj = exjson_object_get_object(object, current_name);
        }
        exparson_free(current_name);
        return exjson_object_dotset_value(temp_obj, dot_pos + 1, value);
    }
}

EXJSON_Status exjson_object_dotset_string(EXJSON_Object *object, const char *name, const char *string) {
    EXJSON_Value *value = exjson_value_init_string(string);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_object_dotset_value(object, name, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_object_dotset_number(EXJSON_Object *object, const char *name, double number) {
    EXJSON_Value *value = exjson_value_init_number(number);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_object_dotset_value(object, name, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_object_dotset_boolean(EXJSON_Object *object, const char *name, int boolean) {
    EXJSON_Value *value = exjson_value_init_boolean(boolean);
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_object_dotset_value(object, name, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_object_dotset_null(EXJSON_Object *object, const char *name) {
    EXJSON_Value *value = exjson_value_init_null();
    if (value == NULL) {
        return EXJSONFailure;
    }
    if (exjson_object_dotset_value(object, name, value) == EXJSONFailure) {
        exjson_value_free(value);
        return EXJSONFailure;
    }
    return EXJSONSuccess;
}

EXJSON_Status exjson_object_remove(EXJSON_Object *object, const char *name) {
    size_t i = 0, last_item_index = 0;
    if (object == NULL || exjson_object_get_value(object, name) == NULL) {
        return EXJSONFailure;
    }
    last_item_index = exjson_object_get_count(object) - 1;
    for (i = 0; i < exjson_object_get_count(object); i++) {
        if (strcmp(object->names[i], name) == 0) {
            exparson_free(object->names[i]);
            exjson_value_free(object->values[i]);
            if (i != last_item_index) { /* Replace key value pair with one from the end */
                object->names[i] = object->names[last_item_index];
                object->values[i] = object->values[last_item_index];
            }
            object->count -= 1;
            return EXJSONSuccess;
        }
    }
    return EXJSONFailure; /* No execution path should end here */
}

EXJSON_Status exjson_object_dotremove(EXJSON_Object *object, const char *name) {
    const char *dot_pos = strchr(name, '.');
    char *current_name = NULL;
    EXJSON_Object *temp_obj = NULL;
    if (dot_pos == NULL) {
        return exjson_object_remove(object, name);
    } else {
        current_name = exparson_strndup(name, dot_pos - name);
        temp_obj = exjson_object_get_object(object, current_name);
        exparson_free(current_name);
        if (temp_obj == NULL) {
            return EXJSONFailure;
        }
        return exjson_object_dotremove(temp_obj, dot_pos + 1);
    }
}

EXJSON_Status exjson_object_clear(EXJSON_Object *object) {
    size_t i = 0;
    if (object == NULL) {
        return EXJSONFailure;
    }
    for (i = 0; i < exjson_object_get_count(object); i++) {
        exparson_free(object->names[i]);
        exjson_value_free(object->values[i]);
    }
    object->count = 0;
    return EXJSONSuccess;
}

EXJSON_Status exjson_validate(const EXJSON_Value *schema, const EXJSON_Value *value) {
    EXJSON_Value *temp_schema_value = NULL, *temp_value = NULL;
    EXJSON_Array *schema_array = NULL, *value_array = NULL;
    EXJSON_Object *schema_object = NULL, *value_object = NULL;
    EXJSON_Value_Type schema_type = EXJSONError, value_type = EXJSONError;
    const char *key = NULL;
    size_t i = 0, count = 0;
    if (schema == NULL || value == NULL) {
        return EXJSONFailure;
    }
    schema_type = exjson_value_get_type(schema);
    value_type = exjson_value_get_type(value);
    if (schema_type != value_type && schema_type != EXJSONNull) { /* null represents all values */
        return EXJSONFailure;
    }
    switch (schema_type) {
        case EXJSONArray:
            schema_array = exjson_value_get_array(schema);
            value_array = exjson_value_get_array(value);
            count = exjson_array_get_count(schema_array);
            if (count == 0) {
                return EXJSONSuccess; /* Empty array allows all types */
            }
            /* Get first value from array, rest is ignored */
            temp_schema_value = exjson_array_get_value(schema_array, 0);
            for (i = 0; i < exjson_array_get_count(value_array); i++) {
                temp_value = exjson_array_get_value(value_array, i);
                if (exjson_validate(temp_schema_value, temp_value) == EXJSONFailure) {
                    return EXJSONFailure;
                }
            }
            return EXJSONSuccess;
        case EXJSONObject:
            schema_object = exjson_value_get_object(schema);
            value_object = exjson_value_get_object(value);
            count = exjson_object_get_count(schema_object);
            if (count == 0) {
                return EXJSONSuccess; /* Empty object allows all objects */
            } else if (exjson_object_get_count(value_object) < count) {
                return EXJSONFailure; /* Tested object mustn't have less name-value pairs than schema */
            }
            for (i = 0; i < count; i++) {
                key = exjson_object_get_name(schema_object, i);
                temp_schema_value = exjson_object_get_value(schema_object, key);
                temp_value = exjson_object_get_value(value_object, key);
                if (temp_value == NULL) {
                    return EXJSONFailure;
                }
                if (exjson_validate(temp_schema_value, temp_value) == EXJSONFailure) {
                    return EXJSONFailure;
                }
            }
            return EXJSONSuccess;
        case EXJSONString: case EXJSONNumber: case EXJSONBoolean: case EXJSONNull:
            return EXJSONSuccess; /* equality already tested before switch */
        case EXJSONError: default:
            return EXJSONFailure;
    }
}

int exjson_value_equals(const EXJSON_Value *a, const EXJSON_Value *b) {
    EXJSON_Object *a_object = NULL, *b_object = NULL;
    EXJSON_Array *a_array = NULL, *b_array = NULL;
    const char *a_string = NULL, *b_string = NULL;
    const char *key = NULL;
    size_t a_count = 0, b_count = 0, i = 0;
    EXJSON_Value_Type a_type, b_type;
    a_type = exjson_value_get_type(a);
    b_type = exjson_value_get_type(b);
    if (a_type != b_type) {
        return 0;
    }
    switch (a_type) {
        case EXJSONArray:
            a_array = exjson_value_get_array(a);
            b_array = exjson_value_get_array(b);
            a_count = exjson_array_get_count(a_array);
            b_count = exjson_array_get_count(b_array);
            if (a_count != b_count) {
                return 0;
            }
            for (i = 0; i < a_count; i++) {
                if (!exjson_value_equals(exjson_array_get_value(a_array, i),
                                       exjson_array_get_value(b_array, i))) {
                    return 0;
                }
            }
            return 1;
        case EXJSONObject:
            a_object = exjson_value_get_object(a);
            b_object = exjson_value_get_object(b);
            a_count = exjson_object_get_count(a_object);
            b_count = exjson_object_get_count(b_object);
            if (a_count != b_count) {
                return 0;
            }
            for (i = 0; i < a_count; i++) {
                key = exjson_object_get_name(a_object, i);
                if (!exjson_value_equals(exjson_object_get_value(a_object, key),
                                       exjson_object_get_value(b_object, key))) {
                    return 0;
                }
            }
            return 1;
        case EXJSONString:
            a_string = exjson_value_get_string(a);
            b_string = exjson_value_get_string(b);
            if (a_string == NULL || b_string == NULL) {
                return 0; /* shouldn't happen */
            }
            return strcmp(a_string, b_string) == 0;
        case EXJSONBoolean:
            return exjson_value_get_boolean(a) == exjson_value_get_boolean(b);
        case EXJSONNumber:
            return fabs(exjson_value_get_number(a) - exjson_value_get_number(b)) < 0.000001; /* EPSILON */
        case EXJSONError:
            return 1;
        case EXJSONNull:
            return 1;
        default:
            return 1;
    }
}

EXJSON_Value_Type exjson_type(const EXJSON_Value *value) {
    return exjson_value_get_type(value);
}

EXJSON_Object * exjson_object (const EXJSON_Value *value) {
    return exjson_value_get_object(value);
}

EXJSON_Array * exjson_array  (const EXJSON_Value *value) {
    return exjson_value_get_array(value);
}

const char * exjson_string (const EXJSON_Value *value) {
    return exjson_value_get_string(value);
}

double exjson_number (const EXJSON_Value *value) {
    return exjson_value_get_number(value);
}

int exjson_boolean(const EXJSON_Value *value) {
    return exjson_value_get_boolean(value);
}

void exjson_set_allocation_functions(EXJSON_Malloc_Function malloc_fun, EXJSON_Free_Function free_fun) {
    exparson_malloc = malloc_fun;
    exparson_free = free_fun;
}