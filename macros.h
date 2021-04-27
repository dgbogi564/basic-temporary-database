#ifndef P3_MACROS_H
#define P3_MACROS_H

#include <stdio.h>
#include <stdlib.h>

#define err(. . .) fprintf(stderr, __VA_ARGS__);

#define MALLOC(func_name, size): { \
     void *ptr = malloc(size); \
     d                      \
     if (ptr == NULL) {      \
          ERR("%s(): Failed to get\n", func_name) \
          exit(EXIT_FAILURE);   \
     }                      \
     return ptr;            \
}

#endif //P3_MACROS_H
