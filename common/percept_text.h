#pragma once

#include <stddef.h>

#include "brain.h"

/* A percept as one line of words, in the frozen format the model was trained
 * on. See model/schema.md. */

/* Long enough for any line, with room to spare. */
#define PERCEPT_TEXT_MAX 160

/* Write p as a line into buf. Returns the line's length, or -1 if p is
 * outside the schema or buf is too small; buf then holds an empty string. */
int percept_encode(const percept_t *p, char *buf, size_t size);
