#pragma once

/* Shared by the tests that need the model file. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MODEL_PATH "../model/out/deskfish_model.bin"

/* A whole file in memory, for the caller to free. NULL if it cannot be read. */
static inline uint8_t *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)length + 1);
    if (data != NULL && fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)length;
    return data;
}
