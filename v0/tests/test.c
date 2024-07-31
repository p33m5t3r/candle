#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/string_ext.h"
#include "../src/tensor.h"
#include "../src/json.h"


void test_json_build(void) {
    char* strs[] = {"s1", "string2", "str3"};
    size_t n_strs = sizeof(strs) / sizeof(strs[0]);
    JsonObject* o1 = json_init();
    JsonObject* o2 = json_init();
    JsonObject* j = json_init();
    JsonObject* k = json_init();

    json_set_str(o1, "a", "b");
    json_set_str(o2, "c", "d");
    JsonObject* objs[] = {o1, o2};
    size_t n_objs = sizeof(objs) / sizeof(objs[0]);
    json_set_arr(k, "obj_arr", J_OBJ, (void**)objs, n_objs);

    json_set_str(j, "j_k1", "j_v1");
    json_set_arr(j, "str_arr", J_STR, (void**)strs, n_strs);
    json_set_str(k, "k_k1", "k_v1");
    json_set_obj(j, "obj_k", k);
    char* expected = "{\"j_k1\": \"j_v1\", "
                     "\"str_arr\": [\"s1\", \"string2\", \"str3\"],"
                     " \"obj_k\": {\"obj_arr\": [{\"a\": \"b\"}, "
                     "{\"c\": \"d\"}], \"k_k1\": \"k_v1\"}}";

    // TODO: obviously make this testing more organized
    char* strjson = json_dumps(j);
    assert(!strcmp(strjson, expected) && "json construction failed");
    printf("trivial json build OK\n");
    free(strjson);

    char* str_result = NULL;
    expected = "j_v1";
    json_get_str(j, "j_k1", &str_result);
    assert(!strcmp(str_result, expected) && "json_get_str failed");
    printf("json_get_str OK\n");

    json_free(j);
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
    JsonObject* j = json_init();
    json_set_vec(j, "vec_k", v);
    char* j_str = json_dumps(j);
    // printf("%s", j_str);
    free(j_str);
    json_free(j);

    // make JsonObject w/ vec pointing at xs
    // freeing j should also free xs
    v = vec_from_takes(xs, 3);
    j = json_init();
    json_set_vec(j, "vec_k", v);
    j_str = json_dumps(j);
    free(j_str);
    json_free(j);
    }

    float* xs = malloc(3 * sizeof(float));
    xs[0] = 3.14;
    xs[1] = 2.7;
    xs[2] = 1.0;
    Vec* v = vec_from_copy(xs, 3);
    Vec* out = NULL;
    JsonObject* j = json_init();

    json_set_vec(j, "vec", v);
    json_get_vec(j, "vec", &out);
    char* s = vec_to_str(out);
    // printf("%s", s);
    free(s);

    json_free(j);
    free(xs);
    printf("json vec construction OK\n");
}

void test_json_parse(void) {
    char* str = "{\"foo\" : \"bar\", \"r\": 12.3,\
                  \"v\" : [ 0.1,2, 3.14, 4, 5 ] }";
    JsonObject* j = json_init();
    json_parse(j, str);

    char* js = json_dumps(j);
    // printf("%s\n", js);

    json_free(j);
    free(js);
    printf("json parsing OK\n");
}


int main() {
    test_json_build();
    test_json_vec();
    test_json_parse();
}

