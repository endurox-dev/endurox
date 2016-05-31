/*
 Exparson 
 based on parson ( http://kgabis.github.com/parson/ )
 Copyright (c) 2012 - 2015 Krzysztof Gabis
 
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

#ifndef exparson_exparson_h
#define exparson_exparson_h

#ifdef __cplusplus
extern "C"
{
#endif    
    
#include <stddef.h>   /* size_t */    
    
/* Types and enums */
typedef struct exjson_object_t EXJSON_Object;
typedef struct exjson_array_t  EXJSON_Array;
typedef struct exjson_value_t  EXJSON_Value;

enum exjson_value_type {
    EXJSONError   = -1,
    EXJSONNull    = 1,
    EXJSONString  = 2,
    EXJSONNumber  = 3,
    EXJSONObject  = 4,
    EXJSONArray   = 5,
    EXJSONBoolean = 6
};
typedef int EXJSON_Value_Type;
    
enum exjson_result_t {
    EXJSONSuccess = 0,
    EXJSONFailure = -1
};
typedef int EXJSON_Status;
    
typedef void * (*EXJSON_Malloc_Function)(size_t);
typedef void   (*EXJSON_Free_Function)(void *);

/* Call only once, before calling any other function from exparson API. If not called, malloc and free
   from stdlib will be used for all allocations */
void exjson_set_allocation_functions(EXJSON_Malloc_Function malloc_fun, EXJSON_Free_Function free_fun);
    
/* Parses first EXJSON value in a file, returns NULL in case of error */
EXJSON_Value * exjson_parse_file(const char *filename);

/* Parses first EXJSON value in a file and ignores comments (/ * * / and //),
   returns NULL in case of error */
EXJSON_Value * exjson_parse_file_with_comments(const char *filename);
    
/*  Parses first EXJSON value in a string, returns NULL in case of error */
EXJSON_Value * exjson_parse_string(const char *string);

/*  Parses first EXJSON value in a string and ignores comments (/ * * / and //),
    returns NULL in case of error */
EXJSON_Value * exjson_parse_string_with_comments(const char *string);
    
/* Serialization */
size_t      exjson_serialization_size(const EXJSON_Value *value); /* returns 0 on fail */
EXJSON_Status exjson_serialize_to_buffer(const EXJSON_Value *value, char *buf, size_t buf_size_in_bytes);
EXJSON_Status exjson_serialize_to_file(const EXJSON_Value *value, const char *filename);
char *      exjson_serialize_to_string(const EXJSON_Value *value);

/* Pretty serialization */
size_t      exjson_serialization_size_pretty(const EXJSON_Value *value); /* returns 0 on fail */
EXJSON_Status exjson_serialize_to_buffer_pretty(const EXJSON_Value *value, char *buf, size_t buf_size_in_bytes);
EXJSON_Status exjson_serialize_to_file_pretty(const EXJSON_Value *value, const char *filename);
char *      exjson_serialize_to_string_pretty(const EXJSON_Value *value);

void        exjson_free_serialized_string(char *string); /* frees string from exjson_serialize_to_string and exjson_serialize_to_string_pretty */

/* Comparing */
int  exjson_value_equals(const EXJSON_Value *a, const EXJSON_Value *b);
    
/* Validation
   This is *NOT* EXJSON Schema. It validates exjson by checking if object have identically 
   named fields with matching types.
   For example schema {"name":"", "age":0} will validate 
   {"name":"Joe", "age":25} and {"name":"Joe", "age":25, "gender":"m"},
   but not {"name":"Joe"} or {"name":"Joe", "age":"Cucumber"}.
   In case of arrays, only first value in schema is checked against all values in tested array.
   Empty objects ({}) validate all objects, empty arrays ([]) validate all arrays,
   null validates values of every type.
 */
EXJSON_Status exjson_validate(const EXJSON_Value *schema, const EXJSON_Value *value);
    
/*
 * EXJSON Object
 */
EXJSON_Value  * exjson_object_get_value  (const EXJSON_Object *object, const char *name);
const char  * exjson_object_get_string (const EXJSON_Object *object, const char *name);
const char  * exjson_object_get_string_n (const EXJSON_Object *object, size_t n);
EXJSON_Object * exjson_object_get_object (const EXJSON_Object *object, const char *name);
EXJSON_Array  * exjson_object_get_array  (const EXJSON_Object *object, const char *name);
double        exjson_object_get_number (const EXJSON_Object *object, const char *name); /* returns 0 on fail */
double        exjson_object_get_number_n (const EXJSON_Object *object, size_t n); /* returns 0 on fail */
int           exjson_object_get_boolean(const EXJSON_Object *object, const char *name); /* returns -1 on fail */
int           exjson_object_get_boolean_n(const EXJSON_Object *object, size_t n); /* returns -1 on fail */

/* dotget functions enable addressing values with dot notation in nested objects,
 just like in structs or c++/java/c# objects (e.g. objectA.objectB.value).
 Because valid names in EXJSON can contain dots, some values may be inaccessible
 this way. */
EXJSON_Value  * exjson_object_dotget_value  (const EXJSON_Object *object, const char *name);
const char  * exjson_object_dotget_string (const EXJSON_Object *object, const char *name);
EXJSON_Object * exjson_object_dotget_object (const EXJSON_Object *object, const char *name);
EXJSON_Array  * exjson_object_dotget_array  (const EXJSON_Object *object, const char *name);
double        exjson_object_dotget_number (const EXJSON_Object *object, const char *name); /* returns 0 on fail */
int           exjson_object_dotget_boolean(const EXJSON_Object *object, const char *name); /* returns -1 on fail */

/* Functions to get available names */
size_t        exjson_object_get_count(const EXJSON_Object *object);
const char  * exjson_object_get_name (const EXJSON_Object *object, size_t index);
    
/* Creates new name-value pair or frees and replaces old value with a new one. 
 * exjson_object_set_value does not copy passed value so it shouldn't be freed afterwards. */
EXJSON_Status exjson_object_set_value(EXJSON_Object *object, const char *name, EXJSON_Value *value);
EXJSON_Status exjson_object_set_string(EXJSON_Object *object, const char *name, const char *string);
EXJSON_Status exjson_object_set_array(EXJSON_Object *object, const char *name, EXJSON_Array *array);
EXJSON_Status exjson_object_set_number(EXJSON_Object *object, const char *name, double number);
EXJSON_Status exjson_object_set_boolean(EXJSON_Object *object, const char *name, int boolean);
EXJSON_Status exjson_object_set_null(EXJSON_Object *object, const char *name);

/* Works like dotget functions, but creates whole hierarchy if necessary.
 * exjson_object_dotset_value does not copy passed value so it shouldn't be freed afterwards. */
EXJSON_Status exjson_object_dotset_value(EXJSON_Object *object, const char *name, EXJSON_Value *value);
EXJSON_Status exjson_object_dotset_string(EXJSON_Object *object, const char *name, const char *string);
EXJSON_Status exjson_object_dotset_number(EXJSON_Object *object, const char *name, double number);
EXJSON_Status exjson_object_dotset_boolean(EXJSON_Object *object, const char *name, int boolean);
EXJSON_Status exjson_object_dotset_null(EXJSON_Object *object, const char *name);

/* Frees and removes name-value pair */
EXJSON_Status exjson_object_remove(EXJSON_Object *object, const char *name);

/* Works like dotget function, but removes name-value pair only on exact match. */
EXJSON_Status exjson_object_dotremove(EXJSON_Object *object, const char *key);

/* Removes all name-value pairs in object */
EXJSON_Status exjson_object_clear(EXJSON_Object *object);
    
/* 
 *EXJSON Array 
 */
EXJSON_Value  * exjson_array_get_value  (const EXJSON_Array *array, size_t index);
const char  * exjson_array_get_string (const EXJSON_Array *array, size_t index);
EXJSON_Object * exjson_array_get_object (const EXJSON_Array *array, size_t index);
EXJSON_Array  * exjson_array_get_array  (const EXJSON_Array *array, size_t index);
double        exjson_array_get_number (const EXJSON_Array *array, size_t index); /* returns 0 on fail */
int           exjson_array_get_boolean(const EXJSON_Array *array, size_t index); /* returns -1 on fail */
size_t        exjson_array_get_count  (const EXJSON_Array *array);
    
/* Frees and removes value at given index, does nothing and returns EXJSONFailure if index doesn't exist.
 * Order of values in array may change during execution.  */
EXJSON_Status exjson_array_remove(EXJSON_Array *array, size_t i);

/* Frees and removes from array value at given index and replaces it with given one.
 * Does nothing and returns EXJSONFailure if index doesn't exist. 
 * exjson_array_replace_value does not copy passed value so it shouldn't be freed afterwards. */
EXJSON_Status exjson_array_replace_value(EXJSON_Array *array, size_t i, EXJSON_Value *value);
EXJSON_Status exjson_array_replace_string(EXJSON_Array *array, size_t i, const char* string);
EXJSON_Status exjson_array_replace_number(EXJSON_Array *array, size_t i, double number);
EXJSON_Status exjson_array_replace_boolean(EXJSON_Array *array, size_t i, int boolean);
EXJSON_Status exjson_array_replace_null(EXJSON_Array *array, size_t i);

/* Frees and removes all values from array */
EXJSON_Status exjson_array_clear(EXJSON_Array *array);

/* Appends new value at the end of array.
 * exjson_array_append_value does not copy passed value so it shouldn't be freed afterwards. */
EXJSON_Status exjson_array_append_value(EXJSON_Array *array, EXJSON_Value *value);
EXJSON_Status exjson_array_append_string(EXJSON_Array *array, const char *string);
EXJSON_Status exjson_array_append_number(EXJSON_Array *array, double number);
EXJSON_Status exjson_array_append_boolean(EXJSON_Array *array, int boolean);
EXJSON_Status exjson_array_append_null(EXJSON_Array *array);
    
/*
 *EXJSON Value
 */
EXJSON_Value * exjson_value_init_object (void);
EXJSON_Value * exjson_value_init_array  (void);
EXJSON_Value * exjson_value_init_string (const char *string); /* copies passed string */
EXJSON_Value * exjson_value_init_number (double number);
EXJSON_Value * exjson_value_init_boolean(int boolean);
EXJSON_Value * exjson_value_init_null   (void);
EXJSON_Value * exjson_value_deep_copy   (const EXJSON_Value *value);
void         exjson_value_free        (EXJSON_Value *value);

EXJSON_Value_Type exjson_value_get_type   (const EXJSON_Value *value);
EXJSON_Object *   exjson_value_get_object (const EXJSON_Value *value);
EXJSON_Array  *   exjson_value_get_array  (const EXJSON_Value *value);
const char  *   exjson_value_get_string (const EXJSON_Value *value);
double          exjson_value_get_number (const EXJSON_Value *value);
int             exjson_value_get_boolean(const EXJSON_Value *value);

/* Same as above, but shorter */
EXJSON_Value_Type exjson_type   (const EXJSON_Value *value);
EXJSON_Object *   exjson_object (const EXJSON_Value *value);
EXJSON_Array  *   exjson_array  (const EXJSON_Value *value);
const char  *   exjson_string (const EXJSON_Value *value);
double          exjson_number (const EXJSON_Value *value);
int             exjson_boolean(const EXJSON_Value *value);

/* Madars... */
EXJSON_Value * exjson_object_get_nvalue_n(const EXJSON_Object *object, size_t n);
EXJSON_Value * exjson_object_nget_value_n(const EXJSON_Object *object, size_t n);

EXJSON_Array * exjson_array_init(void);

#ifdef __cplusplus
}
#endif

#endif
