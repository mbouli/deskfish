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

/* Answer a fish's situation: the best-scoring goal, then the best-scoring
 * mood. False, with *out untouched, if p is outside the schema. Not
 * re-entrant: one model, one caller at a time. */
bool model_choose(model_t *m, const percept_t *p, choice_t *out);

/* The model gives the same answer to the same situation every time, so it
 * keeps recent answers and gives them again without working them out. This
 * is on to begin with; turning it off, or on again, forgets them all. */
void model_remember(model_t *m, bool on);
unsigned model_recalled(const model_t *m); /* answers given from memory so far */

/* Where models loaded from now on keep their numbers: for a board with a
 * small fast memory and a large slow one. NULL for both is the ordinary kind. */
void model_use_memory(void *(*alloc)(size_t size), void (*release)(void *p));
