#ifndef STRING_EXT_H
#define STRING_EXT_H

#include <stdlib.h>
#define DEFAULT_STRING_CAPACITY 8

typedef struct {
    char* data;
    size_t capacity;
    size_t length;
} String;

String* string_new(size_t capacity);
String* string_from(const char* src);
String* string_concat(const String* str1, const String* str2);
void string_free(String* str);
void string_print(const String* str);
void string_append(String* dst, const char* src);
void string_prepend(String* dst, const char* src);
int string_len(const String* str);
int string_cmp(const String* s1, const String* s2);
char* string_to_chars(const String* str);
char* strdup_local(const char* s);

#endif // STRING_EXT_H
