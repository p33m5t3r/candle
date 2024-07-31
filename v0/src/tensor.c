#include "tensor.h"
#include "string_ext.h"     // for string repr
#include <string.h>         // memcpy
#include <stdio.h>          // snprintf
#include <float.h>          // DBL_DECIMAL_DIG

/* allocates new vec, caller owns returned vec */
Vec* vec_init(size_t dim) {
    Vec* vec = malloc(sizeof(Vec));
    if (vec == NULL) return NULL;
    
    vec->dim = dim;
    vec->data = malloc(dim * sizeof(float));
    return vec;
}

/* copies data, caller owns returned vec */
Vec* vec_from_copy(const float* data, size_t dim) {
    Vec* v = vec_init(dim);
    if (v == NULL) return NULL;

    memcpy(v->data, data, dim * sizeof(float)); 
    return v;
}
/* references data, caller owns returned vec */
Vec* vec_from_takes(float* data, size_t dim) {
    Vec* vec = malloc(sizeof(Vec));
    if (vec == NULL) return NULL;
    vec->dim = dim;
    vec->data = data;
    return vec;
}

/* frees vec and its data */
void vec_free(Vec* v) {
    if (v == NULL) return;
    if (v->data) free(v->data);
    free(v);
}

/* returns new string representation, caller must free */
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



