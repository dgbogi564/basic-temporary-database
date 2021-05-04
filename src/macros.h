#ifndef P3_MACROS_H
#define P3_MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

#define eprintf(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

void* safe_malloc(const char func_name[], size_t size);

#endif //P3_MACROS_H