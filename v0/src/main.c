#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <ctype.h>
#include "string_ext.h"

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


void json_object_free(JsonObject* obj);
static void json_value_free(JsonValue* value);
static void json_value_free_inner(JsonValue* value);
static char* json_value_str(JsonValue* value);

/* initializes an empty JsonArray with given `initial_capacity` */
JsonArray* json_array_init(size_t initial_capacity) {
    JsonArray* arr = malloc(sizeof(JsonArray));
    if (arr == NULL) return NULL;
    arr->values = malloc(initial_capacity * sizeof(JsonValue));
    if (arr->values == NULL) {
        free(arr);
        return NULL;
    }
    arr->capacity = initial_capacity;
    arr->size = 0;
    return arr;
}

static void json_array_free(JsonArray* arr) {
    if (arr == NULL) return;

    for (size_t i = 0; i < arr->size; i++)
        json_value_free_inner(arr->values + i);

    free(arr->values);
    free(arr);
}

static bool json_array_resize(JsonArray* arr, size_t new_capacity) {
    JsonValue* new_values = realloc(arr->values, new_capacity * sizeof(JsonValue));
    if (new_values == NULL) return false;
    arr->values = new_values;
    arr->capacity = new_capacity;
    return true;
}

static bool json_array_resize_if_needed(JsonArray* arr, size_t new_size) {
    if (arr->capacity > new_size) return true;  // ok, nothing to do
    size_t new_capacity = arr->capacity;
    while (new_capacity <= new_size) new_capacity *= 2;
    return json_array_resize(arr, new_capacity);
}

static bool json_array_append(JsonArray* arr, JsonValue* value) {
    json_array_resize_if_needed(arr, arr->size + 1);
    arr->values[arr->size++] = *value;
    return true;
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

// public api
JsonObject* json_object_init() {
    JsonObject* obj = malloc(sizeof(JsonObject));
    if (obj) {
        obj->head = NULL;
    }
    return obj;
}

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
        // arr->values[i] = *values[i];
        json_array_append(arr, values[i]);
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

// copies the key and the value into the json object as a pair
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

JsonValue* json_object_get(const JsonObject* obj, const char* k) {
    if (obj == NULL || obj->head == NULL) return NULL;
    JsonPair* current = obj->head;
    while(current != NULL) {
        if (strcmp(current->key, k) == 0) return current->value;
        current = current->next;
    }
    return NULL;
}

JsonValue* json_object_get_typecheck(const JsonObject* obj, 
        const char* k, JsonType t) {
    JsonValue* val = json_object_get(obj, k);
    if (val == NULL || val->type != t) return NULL;
    return val;
}

/* returns a pointer to the char* data in the json object if its type matches */
bool json_object_get_str(const JsonObject* obj, const char* k, char** out) {
    JsonValue* val = json_object_get_typecheck(obj, k, J_STR);
    if (val == NULL) return false;
    *out = val->value.string;
    return true;
}

/* returns a pointer to the Vec* in the json object if its type matches */
bool json_object_get_vec(const JsonObject* obj, const char* k, Vec** out) {
    JsonValue* val = json_object_get_typecheck(obj, k, J_VEC);
    if (val == NULL) return false;
    *out = val->value.vec;
    return true;
}

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

/* context for the json parser */
typedef struct {
    const char* data;
    size_t loc;
    size_t len;
} JsonSrc;

typedef enum {
    SUCCESS = 0,
    OOM = -1,
    INVALID_JSON = -2,
} ParserResultCode;

static bool has_ch(JsonSrc* src) {
    return src->loc <= src->len;
}

static char peek_ch(JsonSrc* src) {
    return has_ch(src) ? src->data[src->loc] : '\0';
}

static char consume_ch(JsonSrc* src) {
    return has_ch(src) ? src->data[src->loc++] : '\0';
}

static void consume_if_eq(JsonSrc* src, char ch) {
    if (peek_ch(src) == ch) consume_ch(src);
}

static bool next_isnt(char ch, JsonSrc* src) {
    return peek_ch(src) != ch;
}

static void skip_whitespace(JsonSrc* src) {
    while (isspace(peek_ch(src))) consume_ch(src);
}

static size_t count_ch_until(JsonSrc* src, char to_count, char stop) {
    size_t count = 0, loc = src->loc;
    char cur_ch;
    while (loc <= src->len && (cur_ch = *(src->data + loc++)) != stop)
        count += (cur_ch == to_count) ? 1 : 0;
    return count;
}

int parse_string_into(JsonSrc* src, char** dst) {
    // TAKE '"'
    skip_whitespace(src);
    if (next_isnt('"', src)) return INVALID_JSON;
    consume_ch(src);

    size_t start_loc = src->loc;
    size_t len = 0;
    bool escaped = false;

    while (has_ch(src)) {
        char c = consume_ch(src);
        if (escaped) {
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            char* result = malloc(len + 1);
            if (result == NULL) return OOM;
            memcpy(result, src->data + start_loc, len);
            result[len] = '\0';
            *dst = result;
            return SUCCESS;
        }
        len++;
    }
    return INVALID_JSON;
}

static int json_value_parse(JsonValue* dst, JsonSrc* src);

static int json_parse_object(JsonObject* dst, JsonSrc* src) {
    // TAKE '{'
    skip_whitespace(src);
    if (next_isnt('{', src)) return INVALID_JSON;
    consume_ch(src);

    skip_whitespace(src);
    while (next_isnt('}', src)) {
        // parse key
        char* key;
        int res = parse_string_into(src, &key);
        if (res != SUCCESS) return res;
        
        // TAKE ':'
        skip_whitespace(src);
        if (next_isnt(':', src)) return INVALID_JSON;
        consume_ch(src);

        // parse value
        JsonValue* value = malloc(sizeof(JsonValue));
        res = json_value_parse(value, src);
        if (res != SUCCESS) return res;

        // add kv pair
        JsonPair* pair = malloc(sizeof(JsonPair));
        pair->value = value;
        pair->key = key;
        pair->next = NULL;
        json_object_add_pair(dst, pair);

        // consume till next key
        skip_whitespace(src);
        consume_if_eq(src, ',');
        skip_whitespace(src);
    }
    return SUCCESS;
}

static int parse_jv_str(JsonValue* dst, JsonSrc* src) {
    char* s;
    int result = parse_string_into(src, &s);
    if (result != SUCCESS) return INVALID_JSON;

    dst->type = J_STR;
    dst->value.string = s;
    return SUCCESS;
}

static int parse_jv_literal(JsonValue* dst, JsonSrc* src) {
    // this is not great but whatever
    skip_whitespace(src);
    size_t start_loc = src->loc;
    size_t len = 0;
    while (has_ch(src)) {
        char c = consume_ch(src);
        if (!isalnum(c) && c != '.') {
            char* result = malloc(len + 1);
            if (result == NULL) return OOM;
            memcpy(result, src->data + start_loc, len);
            result[len] = '\0';
            dst->type = J_STR;
            dst->value.string = result;
            return SUCCESS;
        }
        len++;
    }
    return INVALID_JSON;
}

static int parse_jv_vec(JsonValue* dst, JsonSrc* src) {
    // NOTE: assumes src->loc points at a numeric digit
    // ... ie whitespace has already been cleared by caller
    
    size_t vec_len = count_ch_until(src, ',', ']') + 1;
    Vec* vec = vec_init(vec_len);
    if (vec == NULL) return OOM;

    size_t index = 0;
    while (next_isnt(']', src) && index < vec_len) {
        consume_if_eq(src, ',');
        skip_whitespace(src);
        
        const char* start = src->data + src->loc;
        char* end;
        float value = strtof(start, &end);
        if (start == end) {
            vec_free(vec);
            return INVALID_JSON;
        }
        vec->data[index++] = value;
        src->loc += (end - start);

        skip_whitespace(src);
    }

    consume_if_eq(src, ']');
    dst->type = J_VEC;
    dst->value.vec = vec;
    return SUCCESS;
}

static int parse_jv_arr(JsonValue* dst, JsonSrc* src) {
    // TAKE '['
    skip_whitespace(src);
    if (next_isnt('[', src)) return INVALID_JSON;
    consume_ch(src);
   
    // if it's a flat array and first digit is numeric:
    // ... try parse as vec
    skip_whitespace(src);
    if (isdigit(peek_ch(src))) return parse_jv_vec(dst, src);

    // otherwise, parse into an array
    JsonArray* arr = json_array_init(16);
    while (next_isnt(']', src)) {
        // kill off preceeding comma and whitespace
        consume_if_eq(src, ',');
        skip_whitespace(src);

        // parse the immediate value and append
        JsonValue* value = malloc(sizeof(JsonValue));
        int res = json_value_parse(value, src);
        if (res != SUCCESS) return INVALID_JSON;
        json_array_append(arr, value);
        free(value); // array takes ownership

        skip_whitespace(src);
    }
    consume_if_eq(src, ']');
    dst->type = J_ARR;
    dst->value.arr = arr;
    
    return SUCCESS;
}

/* dst should be allocated*/
static int json_value_parse(JsonValue* dst, JsonSrc* src) {
    skip_whitespace(src);
    if (!has_ch(src)) return INVALID_JSON;

    int result;
    switch(peek_ch(src)) {
        case '"':   
            result = parse_jv_str(dst, src);
            break;
        case '[':   // parses as Vec on non-nested numeric data
            result = parse_jv_arr(dst, src);
            break;
        default:
            result = parse_jv_literal(dst, src);
            break;
    }
    return result;
}

// parse a char* `src` into a JsonObject* `obj` if possible
int json_parse(JsonObject* obj, const char* str) {
    JsonSrc src = { str, 0, strlen(str) };
    return json_parse_object(obj, &src);
}


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

void test_json_build(void) {
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

    // TODO: obviously make this testing more organized
    char* strjson = json_object_dumps(j);
    assert(!strcmp(strjson, expected) && "json construction failed");
    printf("trivial json build OK\n");
    free(strjson);

    char* str_result = NULL;
    expected = "j_v1";
    json_object_get_str(j, "j_k1", &str_result);
    assert(!strcmp(str_result, expected) && "json_get_str failed");
    printf("json_get_str OK\n");

    json_object_free(j);
}

void test_json_vec(void) {
    {   // arbitrary scope to let me re-declare xs later
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
    // printf("%s", j_str);
    free(j_str);
    json_object_free(j);

    // make JsonObject w/ vec pointing at xs
    // freeing j should also free xs
    v = vec_from_takes(xs, 3);
    j = json_object_init();
    json_object_set_vec(j, "vec_k", v);
    j_str = json_object_dumps(j);
    free(j_str);
    json_object_free(j);
    }

    float* xs = malloc(3 * sizeof(float));
    xs[0] = 3.14;
    xs[1] = 2.7;
    xs[2] = 1.0;
    Vec* v = vec_from_copy(xs, 3);
    Vec* out = NULL;
    JsonObject* j = json_object_init();

    json_object_set_vec(j, "vec", v);
    json_object_get_vec(j, "vec", &out);
    char* s = vec_to_str(out);
    // printf("%s", s);
    free(s);

    json_object_free(j);
    free(xs);
    printf("json vec construction OK\n");
}

void test_json_parse(void) {
    char* str = "{\"foo\" : \"bar\", \"r\": 12.3,\
                  \"v\" : [ 0.1,2, 3.14, 4, 5 ] }";
    JsonObject* j = json_object_init();
    json_parse(j, str);

    char* js = json_object_dumps(j);
    printf("%s\n", js);

    json_object_free(j);
    free(js);
    printf("json parsing OK\n");
}


// TODO: parsing (just arrays and make sure recursion plugs for objs)
//       parse vec as tensor w/ strides given by shape vec
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

    // test_json_build();
    // test_json_vec();
   
    test_json_build();
    test_json_vec();
    test_json_parse();
   
    /*
    char* s = "12.4, 123]";
    JsonSrc src = {s, 0, strlen(s)};
    size_t x = count_ch_until(&src, ',', ']');
    printf("%d\n\n\n", (int)x);

    const char* ss = "12.12341, 123]";
    char* end;
    float value = strtof(ss, &end);
    printf("%f\n", value);
    printf("%s\n", end);
    */
}


