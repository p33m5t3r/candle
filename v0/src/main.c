#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// strings
typedef struct {
    char* data;
    size_t capacity;
    size_t length;
} String;

#define DEFAULT_STRING_CAPACITY 8
String* string_new(size_t capacity) {
    String* s = malloc(sizeof(String));
    if (!s) return NULL;
    s->capacity = (capacity <= 1) ? DEFAULT_STRING_CAPACITY: capacity;
    s->data = malloc(s->capacity);
    *(s->data) = '\0';      // null-termination:
    s->length = 0;          // requires capacity > length (strictly!)
    return s;
}

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

String* string_from(const char* src) {
    String* dst = string_new(0);
    size_t dst_len = strlen(src);
    string_resize_if_needed(dst, dst_len);
    strcpy(dst->data, src);
    dst->length = dst_len;
    return dst;
}

void string_free(String* str) {
    free(str->data);
    free(str);
}
void string_print(const String* str) {
    (str && str->data) ?
        printf("%.*s\n", (int)str->length, str->data)
        : printf("NULL\n");
}
void string_append(String* dst, const char* src) {
    size_t new_len = dst->length + strlen(src);
    string_resize_if_needed(dst, new_len);
    strcpy(dst->data + dst->length, src);
    dst->length = new_len;
}
void string_prepend(String* dst, const char* src) {
    size_t src_len = strlen(src);
    size_t new_len = dst->length + src_len;
    string_resize_if_needed(dst, new_len);
    memmove(dst->data + src_len, dst->data, dst->length);
    memcpy(dst->data, src, src_len);
    dst->length = new_len;
    dst->data[new_len] = '\0';
}

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

int string_len(const String* str) {
    return str->length;
}

int string_cmp(const String* s1, const String* s2) {
    return strcmp(s1->data, s2->data);
}

// {"foo": [1,2,3], "bar": {"a": 1, "b": 2}}
typedef enum {
    J_OBJ,      // a (linked) list of key value pairs
    J_ARR,      // a list of values
    J_STR,      // we treat everything as a string
} JsonType;

typedef struct JsonArray JsonArray;
typedef struct JsonObject JsonObject;
typedef struct JsonPair JsonPair;

typedef struct {
    JsonType type;
    union {
        char* string;
        JsonArray* arr;
        JsonObject* obj;
    } value;
} JsonValue;

struct JsonArray{
    JsonValue* values;
    size_t size;
};

struct JsonPair {
    char* key;
    JsonValue* value;
    JsonPair* next;
};

struct JsonObject{
    JsonPair* head;
};

typedef struct {
    double* data;
    size_t dim;
} Vec;

// private api

void json_object_free(JsonObject* obj);
static void json_value_free(JsonValue* value);

static void json_array_free(JsonArray* arr) {
    for (size_t i = 0; i < arr->size; i++)
        json_value_free(arr->values + i);
    free(arr);
}

static void json_value_free(JsonValue* value) {
    if (value == NULL) return;

    switch (value->type) {
        case J_STR:
            free(value->value.string);
            break;
        case J_ARR:
            json_array_free(value->value.arr);
            break;
        case J_OBJ:
            json_object_free(value->value.obj);
            break;
    }
    free(value);
}

static void json_pair_free(JsonPair* head) {
    if (head->key != NULL)   free(head->key);
    if (head->value != NULL) free(head->value);
    if (head->next != NULL)  json_pair_free(head->next);
    free(head);
}

static char* json_array_str(JsonArray* arr);
static char* json_object_str(JsonObject* obj);
static char* json_value_str(JsonValue* value) {
    if (value == NULL) return "NULL";
    switch (value->type) {
        case J_STR:
            return value->value.string;
            break;
        case J_ARR:
            return json_array_str(value->value.arr);
            break;
        default:
            return json_object_str(value->value.obj);
            break;
    }
}

static char* json_array_str(JsonArray* arr) {
    String* s = string_from("[");
    for (size_t i = 0; i < arr->size; i++) {
        if (i > 0) string_append(s, ", ");
        string_append(s, json_value_str(arr->values + i));
    }
    string_append(s, "]");
    return string_to_chars(s);
}

static char* json_object_str(JsonObject* obj) {
    String* s = string_from("{");
    JsonPair* pair = obj->head;
    int count = 0;
    while (pair != NULL) {
        if (count != 0) string_append(s, ", ");
        string_append(s, "\"");
        string_append(s, pair->key);
        string_append(s, "\": ");
        string_append(s, json_value_str(pair->value));
        pair = pair->next;
        count++;
    }
    string_append(s, "}");
    return string_to_chars(s);
}


// public api
JsonObject* json_object_init() {
    return malloc(sizeof(JsonObject));
}
JsonObject* json_object_from(const char* str);  // the parser

void json_object_free(JsonObject* obj) {
    if (obj->head != NULL)
        json_pair_free(obj->head);
    free(obj);
}

// TODO: json_object_set() to set arbitrary json values (traversal)
//       fill out the str and maybe int methods for ease of use
//       try building a simple hardcoded json object
//       make sure printing works, make the json more complicated, repeat
//       write a function to wrap creating a JsonArray
//       put an array in the test object, make sure it works
void json_object_set(JsonObject* obj, const char* k, const JsonValue* v);
void json_object_set_str(JsonObject* obj, const char* k, const char* v);
void json_object_set_int(JsonObject* obj, const char* k, int v);
void json_object_set_bool(JsonObject* obj, const char* k, int v);
void json_object_set_real(JsonObject* obj, const char* k, int v);

JsonValue* json_object_get(const JsonObject* obj, const char* k);
    JsonType json_object_get_type(const JsonObject* obj, const char* k);
    char* json_object_get_str(const JsonObject* obj, const char* k);
    int json_object_get_int(const JsonObject* obj, const char* k);
    int json_object_get_real(const JsonObject* obj, const char* k);
    int json_object_get_bool(const JsonObject* obj, const char* k);
    Vec* json_object_get_vec(const JsonObject* obj, const char* k);

char* json_object_dumps(JsonObject* obj);
void json_object_dump(JsonObject* obj, char* filename);

char* read_file_to_str(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (content == NULL) {
        perror("Memory allocation failed!");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, 1, file_size, file);
    if (read_size != file_size) {
        perror("Error reading file");
        free(content);
        fclose(file);
        return NULL;
    }

    content[file_size] = '\0';
    fclose(file);
    return content;
}

int main() {
    const char FILENAME[] = "test.json";
    char* file_contents = read_file_to_str(FILENAME);
    if (file_contents == NULL) {
        printf("Couldn't find the file: \"%s\". Exiting!", FILENAME);
        return 1;
    }
    printf("%s\n", file_contents);
    return 0;
}


