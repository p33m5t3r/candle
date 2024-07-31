#include "string_ext.h"
#include <stdio.h>  // fprintf, stderr
#include <string.h> // strlen, strcpy, strcmp

/* resizes in-place if needed, may exit on allocation failure */
static void string_resize_if_needed(String* str, size_t new_len) {
    if (str->capacity > new_len) return;
    size_t new_capacity = str->capacity;
    while (new_capacity <= new_len) new_capacity *= 2;
    char* new_data = realloc(str->data, new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "failed to realloc string during resize attempt");
        exit(1);        // if this realloc fails, gg
    }
    str->data = new_data;
    str->capacity = new_capacity;
    str->data[new_len] = '\0';
}

/* allocates new string, caller owns */
String* string_new(size_t capacity) {
    String* s = malloc(sizeof(String));
    if (!s) return NULL;
    s->capacity = (capacity <= 1) ? DEFAULT_STRING_CAPACITY: capacity;
    s->data = malloc(s->capacity);
    *(s->data) = '\0';      // null-termination:
    s->length = 0;          // requires capacity > length (strictly!)
    return s;
}

/* creates new string from c-string, copies data, caller owns */
String* string_from(const char* src) {
    String* dst = string_new(0);
    size_t dst_len = strlen(src);
    string_resize_if_needed(dst, dst_len);
    strcpy(dst->data, src);
    dst->length = dst_len;
    return dst;
}

/* frees string and its data */
void string_free(String* str) {
    free(str->data);
    free(str);
}

/* prints to stdout, safe with null */
void string_print(const String* str) {
    (str && str->data) ?
        printf("%.*s\n", (int)str->length, str->data)
        : printf("NULL\n");
}

/* appends c-string in-place */
void string_append(String* dst, const char* src) {
    size_t new_len = dst->length + strlen(src);
    string_resize_if_needed(dst, new_len);
    strcpy(dst->data + dst->length, src);
    dst->length = new_len;
}

/* prepends c-string in-place */
void string_prepend(String* dst, const char* src) {
    size_t src_len = strlen(src);
    size_t new_len = dst->length + src_len;
    string_resize_if_needed(dst, new_len);
    memmove(dst->data + src_len, dst->data, dst->length);
    memcpy(dst->data, src, src_len);
    dst->length = new_len;
    dst->data[new_len] = '\0';
}

/* creates new string, copies data, caller owns */
String* string_concat(const String* str1, const String* str2) {
    if (str1 == NULL || str2 == NULL) return NULL;
    
    size_t new_length = str1->length + str2->length;
    String* result = string_new(new_length + 1);  // +1 for null terminator
    if (result == NULL) return NULL;

    memcpy(result->data, str1->data, str1->length);
    memcpy(result->data + str1->length, str2->data, str2->length);
    
    result->data[new_length] = '\0';
    result->length = new_length;
    return result;
}

/* creates new c-string from string, copies data, caller owns */
char* string_to_chars(const String* str) {
    if (str == NULL || str->data == NULL) {
        return NULL;
    }
    
    char* copy = malloc(str->length + 1);  // +1 for null terminator
    if (copy == NULL) {
        fprintf(stderr, "Memory allocation failed in string_to_chars");
        return NULL;
    }
    
    strcpy(copy, str->data);
    return copy;
}

/* strlen analog */
int string_len(const String* str) {
    return str->length;
}

/* strcmp analog */
int string_cmp(const String* s1, const String* s2) {
    return strcmp(s1->data, s2->data);
}

/* strdup analog; creates new c-str by copy */
char* strdup_local(const char* s) {
   size_t len = strlen(s) + 1;
   char* new_str = malloc(len);
   if (new_str) {
       memcpy(new_str, s, len);
   }
   return new_str;
}
