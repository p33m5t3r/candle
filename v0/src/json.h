#ifndef JSON_H
#define JSON_H

#include <stdlib.h>  // size_t
#include <stdbool.h> // bool
#include "tensor.h"  // Vec related

typedef enum {
    J_OBJ,      // a (linked) list of key value pairs
    J_ARR,      // a (possibly nested) list of string values or objects
    J_STR,      // every non-obj/arr/vec json value is a string
    J_VEC,      // a 1-d vector of real numbers
} JsonType;

typedef struct JsonArray JsonArray;
typedef struct JsonObject JsonObject;
typedef struct JsonPair JsonPair;

// 16 bytes
typedef struct {
    JsonType type;
    union {
        char* string;
        JsonArray* arr;
        JsonObject* obj;
        Vec* vec;
    } value;
} JsonValue;

// 24 bytes
struct JsonArray {
    JsonValue* values;
    size_t size;
    size_t capacity;
};

// 24 bytes
struct JsonPair {
    char* key;
    JsonValue* value;
    JsonPair* next;
};

// 8 bytes
struct JsonObject {
    JsonPair* head;
};


JsonObject* json_init();
void json_free(JsonObject* obj);

int json_parse(JsonObject* obj, const char* str);
void json_dump(JsonObject* obj, char* filename);
char* json_dumps(JsonObject* obj);

void json_set_vec(JsonObject* obj, const char* k, Vec* v);
void json_set_str(JsonObject* obj, const char* k, const char* v);
void json_set_obj(JsonObject* obj, const char* k, JsonObject* v);
void json_set_arr(JsonObject* obj, const char* k, 
                  JsonType type, void** list, size_t n);

bool json_get_str(const JsonObject* obj, const char* k, char** out);
bool json_get_vec(const JsonObject* obj, const char* k, Vec** out);

#endif // JSON_H
