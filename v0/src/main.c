#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>

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

char* strdup_local(const char* s) {
   size_t len = strlen(s) + 1;
   char* new_str = malloc(len);
   if (new_str) {
       memcpy(new_str, s, len);
   }
   return new_str;
}

typedef struct {
    float* data;
    size_t dim;
} Vec;

Vec* vec_init(size_t dim) {
    Vec* vec = malloc(sizeof(Vec));
    if (vec == NULL) return NULL;
    
    vec->dim = dim;
    vec->data = malloc(dim * sizeof(float));
    return vec;
}

// copies data
Vec* vec_from_copy(const float* data, size_t dim) {
    Vec* v = vec_init(dim);
    if (v == NULL) return NULL;

    memcpy(v->data, data, dim * sizeof(float)); 
    return v;
}
// makes a vec with a reference to data
Vec* vec_from_takes(float* data, size_t dim) {
    Vec* vec = malloc(sizeof(Vec));
    if (vec == NULL) return NULL;
    vec->dim = dim;
    vec->data = data;
    return vec;
}

void vec_free(Vec* v) {
    if (v == NULL) return;
    if (v->data) free(v->data);
    free(v);
}

char* vec_to_str(Vec* v) {
    if (v == NULL || v->data == NULL) return strdup_local("");

    String* s = string_from("[");
    char buffer[32];

    for (size_t i = 0; i < v->dim; i++) {
        if (i > 0) string_append(s, ", ");
        snprintf(buffer, sizeof(buffer), "%.*g", 
                 DBL_DECIMAL_DIG, v->data[i]);
        string_append(s, buffer);
    }
    string_append(s, "]");
    char* result = string_to_chars(s);
    string_free(s);

    return result;
}

// {"foo": [1,2,3], "bar": {"a": 1, "b": 2}}
typedef enum {
    J_OBJ,      // a (linked) list of key value pairs
    J_ARR,      // a (possibly nested) list of string values or objects
    J_STR,      // every non-obj/arr/vec json value is a string
    J_VEC,      // a 1-d vector of real numbers
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
        Vec* vec;
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

// private api

void json_object_free(JsonObject* obj);
static void json_value_free(JsonValue* value);
static void json_value_free_inner(JsonValue* value);

static void json_array_free(JsonArray* arr) {
    if (arr == NULL) return;

    for (size_t i = 0; i < arr->size; i++)
        json_value_free_inner(arr->values + i);

    free(arr->values);
    free(arr);
}

/* frees the data in `value` but not `value` itself */
static void json_value_free_inner(JsonValue* value) {
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
        case J_VEC:
            vec_free(value->value.vec);
            break;
    }
}

/* frees the data in `value` then frees `value` itself */
static void json_value_free(JsonValue* value) {
    if (value == NULL) return;
    json_value_free_inner(value);
    free(value);
}

static void json_pair_free(JsonPair* head) {
    if (head == NULL) return;
    if (head->key != NULL)   free(head->key);
    if (head->value != NULL) json_value_free(head->value);
    json_pair_free(head->next);
    free(head);
}

static char* json_array_str(JsonArray* arr);
static char* json_object_dumps(JsonObject* obj);
static char* json_value_str(JsonValue* value) {
    if (value == NULL) return strdup_local("NULL");
    switch (value->type) {
        case J_STR:
            return strdup_local(value->value.string);
            break;
        case J_ARR:
            return json_array_str(value->value.arr);
            break;
        case J_OBJ:
            return json_object_dumps(value->value.obj);
            break;
        default:    // J_VEC
            return vec_to_str(value->value.vec);
            break;
    }
}

static char* json_array_str(JsonArray* arr) {
    String* s = string_from("[");
    for (size_t i = 0; i < arr->size; i++) {
        if (i > 0) string_append(s, ", ");
        if (arr->values[i].type == J_STR) {
            string_append(s, "\"");
            string_append(s, arr->values[i].value.string);
            string_append(s, "\"");
        } else {
            char* value_str = json_value_str(&arr->values[i]);
            string_append(s, value_str);
            free(value_str);
        }
    }
    string_append(s, "]");
    char* result = string_to_chars(s);
    string_free(s);
    return result;
}

// public api
JsonObject* json_object_init() {
    JsonObject* obj = malloc(sizeof(JsonObject));
    if (obj) {
        obj->head = NULL;
    }
    return obj;
}
JsonObject* json_object_from(const char* str);  // the parser

/* frees the given object, including all nested objects */
void json_object_free(JsonObject* obj) {
    if (obj == NULL) return;
    if (obj->head != NULL) json_pair_free(obj->head);
    free(obj);
}

static JsonPair* json_pair_get_tail(JsonPair* pair) {
    JsonPair* tail = pair;
    while (tail->next != NULL) tail = tail->next;
    return tail;
}

void json_object_add_pair(JsonObject* obj, JsonPair* pair) {
    if (obj->head == NULL) {
        obj->head = pair;
    } else {
        JsonPair* tail = json_pair_get_tail(obj->head);
        tail->next = pair;
    }
    pair->next = NULL;
}

/* initializes an empty JsonArray with space for `size` elements */
JsonArray* json_array_init(size_t size) {
    JsonArray* arr = malloc(sizeof(JsonArray));
    if (arr == NULL) return NULL;
    arr->values = malloc(size * sizeof(JsonValue));
    if (arr->values == NULL) {
        free(arr);
        return NULL;
    }
    arr->size = size;
    return arr;
}

JsonValue** json_values_from(JsonType type, void** list, size_t count) {
    JsonValue** values = malloc(count * sizeof(JsonValue*));
    if (!values) {
        fprintf(stderr, "malloc JsonValue** in json_values_from failed\n");
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        values[i] = malloc(sizeof(JsonValue));
        if (!values[i]) {
            fprintf(stderr, "malloc JsonValue in json_values_from failed\n");
            // Free previously allocated memory
            for (size_t j = 0; j < i; j++) {
                free(values[j]);
            }
            free(values);
            return NULL;
        }

        values[i]->type = type;
        switch (type) {
            case J_STR:
                values[i]->value.string = strdup_local((char*)list[i]);
                break;
            case J_OBJ:
                values[i]->value.obj = (JsonObject*)list[i];
                break;
            default:
                fprintf(stderr, "only J_STR or J_OBJ supported");
                return NULL;
        }
    }

    return values;
}


void json_object_set_arr(JsonObject* obj, const char* k, JsonType type,
                         void** list, size_t n) {
    JsonValue** values = json_values_from(type, list, n);
    JsonArray* arr = json_array_init(n);
    if (arr == NULL) {
        fprintf(stderr, "malloc JsonArray failed!");
        return;
    }
    for (size_t i = 0; i < n; i++) {
        arr->values[i] = *values[i];
        free(values[i]);    // take ownership
    }
    
    JsonValue* arr_value = malloc(sizeof(JsonValue));
    if (arr_value == NULL) {
        fprintf(stderr, "malloc JsonValue failed!");
        json_array_free(arr);
        return;
    }
    arr_value->type = J_ARR;
    arr_value->value.arr = arr;

    JsonPair* pair = malloc(sizeof(JsonPair));
    if (pair == NULL) {
        fprintf(stderr, "malloc JsonPair failed!");
        json_value_free(arr_value);
        return;
    }
    pair->key = strdup_local(k);
    pair->value = arr_value;
    pair->next = NULL;
    json_object_add_pair(obj, pair);

    free(values);
}

void json_object_set_str(JsonObject* obj, const char* k, const char* v) {
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = J_STR;
    value->value.string = strdup_local(v);

    JsonPair* pair = malloc(sizeof(JsonPair));
    pair->key = strdup_local(k);
    pair->value = value;
    pair->next = NULL;

    json_object_add_pair(obj, pair);
}

void json_object_set_obj(JsonObject* obj, const char* k, JsonObject* v) {
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = J_OBJ;
    value->value.obj = v;

    JsonPair* pair = malloc(sizeof(JsonPair));
    pair->key = strdup_local(k);
    pair->value = value;
    pair->next = NULL;

    json_object_add_pair(obj, pair);
}

void json_object_set_vec(JsonObject* obj, const char* k, Vec* v) {
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = J_VEC;
    value->value.vec = v;
    JsonPair* pair = malloc(sizeof(JsonPair));
    pair->key = strdup_local(k);
    pair->value = value;
    pair->next = NULL;

    json_object_add_pair(obj, pair);
}

JsonValue* json_object_get(const JsonObject* obj, const char* k);
    JsonType json_object_get_type(const JsonObject* obj, const char* k);
    char* json_object_get_str(const JsonObject* obj, const char* k);
    int json_object_get_int(const JsonObject* obj, const char* k);
    int json_object_get_real(const JsonObject* obj, const char* k);
    int json_object_get_bool(const JsonObject* obj, const char* k);
    Vec* json_object_get_vec(const JsonObject* obj, const char* k);

char* json_object_dumps(JsonObject* obj) {
    String* s = string_from("{");
    JsonPair* pair = obj->head;
    int count = 0;
    while (pair != NULL) {
        if (count != 0) string_append(s, ", ");
        string_append(s, "\"");
        string_append(s, pair->key);
        string_append(s, "\": ");

        char* value_str = json_value_str(pair->value);
        if (pair->value != NULL && pair->value->type == J_STR) {
            string_append(s, "\"");
            string_append(s, value_str);
            string_append(s, "\"");
        } else {
            string_append(s, value_str);
        }
        free(value_str);

        pair = pair->next;
        count++;
    }
    string_append(s, "}");
    char* out = string_to_chars(s);
    string_free(s);
    return out;
}

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

void test_trivial_json_construction(void) {
    char* strs[] = {"s1", "string2", "str3"};
    size_t n_strs = sizeof(strs) / sizeof(strs[0]);
    JsonObject* o1 = json_object_init();
    JsonObject* o2 = json_object_init();
    JsonObject* j = json_object_init();
    JsonObject* k = json_object_init();

    json_object_set_str(o1, "a", "b");
    json_object_set_str(o2, "c", "d");
    JsonObject* objs[] = {o1, o2};
    size_t n_objs = sizeof(objs) / sizeof(objs[0]);
    json_object_set_arr(k, "obj_arr", J_OBJ, (void**)objs, n_objs);

    json_object_set_str(j, "j_k1", "j_v1");
    json_object_set_arr(j, "str_arr", J_STR, (void**)strs, n_strs);
    json_object_set_str(k, "k_k1", "k_v1");
    json_object_set_obj(j, "obj_k", k);
    char* expected = "{\"j_k1\": \"j_v1\", "
                     "\"str_arr\": [\"s1\", \"string2\", \"str3\"],"
                     " \"obj_k\": {\"obj_arr\": [{\"a\": \"b\"}, "
                     "{\"c\": \"d\"}], \"k_k1\": \"k_v1\"}}";

    char* strjson = json_object_dumps(j);
    assert(!strcmp(strjson, expected) && "error: unexpected json");
    printf("trivial json construction test passed");
    free(strjson);
    json_object_free(j);
}

void test_vec_json_memory(void) {
    float* xs = malloc(3 * sizeof(float));
    xs[0] = 3.14159265359f;
    xs[1] = 2.70f;
    xs[2] = 1.0f;

    // make JsonObject w/ vec owning a copy of xs
    // freeing j won't free xs
    Vec* v = vec_from_copy(xs, 3);
    JsonObject* j = json_object_init();
    json_object_set_vec(j, "vec_k", v);
    char* j_str = json_object_dumps(j);
    printf("%s", j_str);
    free(j_str);
    json_object_free(j);

    // make JsonObject w/ vec pointing at xs
    // freeing j should also free xs
    v = vec_from_takes(xs, 3);
    j = json_object_init();
    json_object_set_vec(j, "vec_k", v);
    j_str = json_object_dumps(j);
    printf("%s", j_str);
    free(j_str);
    json_object_free(j);
}

// TODO: getters
//       parsing
int main() {
    /*
    const char FILENAME[] = "test.json";
    char* file_contents = read_file_to_str(FILENAME);
    if (file_contents == NULL) {
        printf("Couldn't find the file: \"%s\". Exiting!", FILENAME);
        return 1;
    }
    printf("%s\n", file_contents);
    */

    test_trivial_json_construction();
    test_vec_json_memory();

    return 0;
}


