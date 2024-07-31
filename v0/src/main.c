#include <stdio.h>
#include "tensor.h"
#include <stdlib.h>

int main() {
    size_t dim = 3;
    float* data = malloc(sizeof(float) * dim);
    data[0] = 1;
    data[1] = 0;
    data[2] = 0;

    Vec* v = vec_from_takes(data, dim);
    char* v_str = vec_to_str(v);
    printf("%s\n", v_str);

    free(v_str);
    free(data);
    free(v);
}


