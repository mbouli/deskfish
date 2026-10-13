#pragma once

#include <stdio.h>

extern int test_failures;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            test_failures++;                                              \
        }                                                                 \
    } while (0)
