/* inih -- simple .INI file parser

inih is released under the New BSD license (see LICENSE.txt). Go to the project
home page for more info:

https://github.com/benhoyt/inih

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <ndebug.h>
#include <ndrstandard.h>

#include "ndrx_ini.h"
#include "userlog.h"

#if !INI_USE_STACK
#include <stdlib.h>
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

/* Strip whitespace chars off end of given string, in place. Return s. */
static char* rstrip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}

/* Return pointer to first char (of chars) or inline comment in given string,
   or pointer to null at end of string if neither found. Inline comment must
   be prefixed by a whitespace character to register as a comment. */
static char* find_chars_or_comment(const char* s, const char* chars)
{
#if INI_ALLOW_INLINE_COMMENTS
    int was_space = 0;
    while (*s && (!chars || !strchr(chars, *s)) &&
           !(was_space && strchr(INI_INLINE_COMMENT_PREFIXES, *s))) {
        was_space = isspace((unsigned char)(*s));
        s++;
    }
#else
    while (*s && (!chars || !strchr(chars, *s))) {
        s++;
    }
#endif
    return (char*)s;
}

/* Version of strncpy that ensures dest (size bytes) is null-terminated. */
static char* strncpy0(char* dest, const char* src, size_t size)
{
    NDRX_STRNCPY(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

/* See documentation in header file. 
 * mvitolin: TODO: Add support for multi-line value for single call of the handler
 * Needs to do some buffering...
 */
int ndrx_ini_parse_stream(ini_reader reader, void* stream, ini_handler handler,
                     void* user, void *user2, void *user3)
{
    /* Uses a fair bit of stack (use heap instead if you need to) */
#if INI_USE_STACK
    char tmp_line[INI_MAX_LINE];
    char tmp_line2[INI_MAX_LINE];
#else
    char* tmp_line;
    char* tmp_line2;
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;
    
    char *line;
    char *line2;
    

#if !INI_USE_STACK
    line = (char*)NDRX_MALLOC(INI_MAX_LINE);
    if (!line) {
        return -2;
    }
    
    line2 = (char*)NDRX_MALLOC(INI_MAX_LINE);
    if (!line2) {
        return -2;
    }
#endif
    line = tmp_line;
    line2 = tmp_line2;

    /* Scan through stream line by line */
    while (reader(line, INI_MAX_LINE, stream) != NULL) {
        lineno++;

        start = line;
        
#if INI_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));
        
line_buffered:
        if (*start == ';' || *start == '#') {
            /* Per Python configparser, allow both ; and # comments at the
               start of a line */
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start && start > line) {
            /* Non-blank line with leading whitespace, treat as continuation
               of previous name's value (as per Python configparser). */
            if (!handler(user, user2, user3, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == '[') {
            /* A "[section]" line */
            end = find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
            }
            else if (!error) {
                /* No ']' found on section line */
                error = lineno;
            }
        }
        else if (*start) {
            /* Not a comment, must be a name[=:]value pair */
            end = find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
#if INI_ALLOW_INLINE_COMMENTS
                end = find_chars_or_comment(value, NULL);
                if (*end)
                    *end = '\0';
#endif
                rstrip(value);

                /* Valid name[=:]value pair found, call handler */
                strncpy0(prev_name, name, sizeof(prev_name));
                
#if INI_ALLOW_MULTILINE
                /*
                 * Home some more code for multi-line support, buffer the line read...
                 */
                
                while (reader(line2, INI_MAX_LINE, stream) != NULL)
                {
                    lineno++;

                    start = line2;
                    start = lskip(rstrip(start));
                    
                    if (*start == ';' || *start == '#') {
                        /* Per Python configparser, allow both ; and # comments at the
                           start of a line */
                        continue; /* Skip the line... */
                    }
                    else if (*start && start > line2) {
                        int free_space_in_value;
                        int additional_value_len;
                        /* we have an additional data */
                        rstrip(start);
                        #if INI_ALLOW_INLINE_COMMENTS
                                end = find_chars_or_comment(start, NULL);
                                if (*end)
                                    *end = '\0';
                        #endif
                        /* calculate free space in dest buffer */
                        free_space_in_value = INI_MAX_LINE - ((value+strlen(value)) - line);
                        additional_value_len = strlen(start);
                        
                        if (free_space_in_value < additional_value_len)
                        {
                            userlog("Failed to parse config - value too large,"
                                    "config param: %s (limit:%d) runs over by: %d", 
                                    name, INI_MAX_LINE, additional_value_len,
                                    free_space_in_value);
                            error = lineno;
                        }
                        else
                        {
                            strcat(value, start);
                        }
                    }
                    else
                    {
                        /* Send value to user and continue with next line */
                        if (!handler(user, user2, user3, section, name, value) && !error)
                        {
                            error = lineno;
                        }
                        else
                        {
                            /* so we have finished with value
                             * swap the pointers...
                             */
                            char *p;
                            
                            p = line;
                            line = line2;
                            line2 = p;
                            
                            goto line_buffered;
                        }
                    }
                }
                
                /* so if we get here, this was last line, process it accordingly.. */
                if (!handler(user, user2, user3, section, name, value) && !error)
                {
                    error = lineno;
                }
                
#else
                if (!handler(user, user2, user3, section, name, value) && !error)
                    error = lineno;
#endif
                

            }
            else if (!error) {
                /* No '=' or ':' found on name[=:]value line */
                error = lineno;
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }

#if !INI_USE_STACK
    NDRX_FREE(line);
    NDRX_FREE(line2);
#endif

    return error;
}

/* See documentation in header file. */
int ndrx_ini_parse_file(FILE* file, ini_handler handler, void* user, void *user2, 
        void *user3)
{
    return ndrx_ini_parse_stream((ini_reader)fgets, file, handler, user, user2, user3);
}

/* See documentation in header file. */
int ndrx_ini_parse(const char* filename, ini_handler handler, void* user, 
        void *user2, void *user3)
{
    FILE* file;
    int error;

    file = NDRX_FOPEN(filename, "r");
    if (!file)
        return -1;
    error = ndrx_ini_parse_file(file, handler, user, user2, user3);
    NDRX_FCLOSE(file);
    return error;
}
