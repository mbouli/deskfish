#pragma once

#include <stdint.h>

#include "display.h"

/* All functions draw into a DISPLAY_W x DISPLAY_H buffer of host-order RGB565
 * and clip to it: nothing is ever written outside the buffer. */

void gfx_fill(uint16_t *fb, uint16_t colour);
void gfx_fill_rect(uint16_t *fb, int x, int y, int w, int h, uint16_t colour);
