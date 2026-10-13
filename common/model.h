#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "brain.h"

/* The student model: a small transformer read from a DFM1 file (see the top
 * of model/export.py for the format). */
typedef struct model model_t;

/* Read a model from the bytes of a file. NULL if it is not a usable model
 * file for this schema, or if memory ran out. The bytes are not kept. */
model_t *model_load(const uint8_t *data, size_t size);
void model_free(model_t *m);

/* How many learned numbers it holds. */
size_t model_numbers(const model_t *m);
