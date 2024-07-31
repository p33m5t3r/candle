#ifndef TENSOR_H
#define TENSOR_H

#include <stdlib.h> // size_t

typedef struct {
    float* data;
    size_t dim;
} Vec;


Vec* vec_init(size_t dim);
Vec* vec_from_copy(const float* data, size_t dim);
Vec* vec_from_takes(float* data, size_t dim);
void vec_free(Vec* v);
char* vec_to_str(Vec* v);


#endif // TENSOR_H
