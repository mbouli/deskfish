#pragma once

#include <stdint.h>

#define DISPLAY_W 320
#define DISPLAY_H 240

/* A host-order RGB565 pixel from 0-255 components. */
#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

/* Start the screen and turn the backlight on. Aborts with a serial error on failure. */
void display_init(void);

/* Send a w x h rectangle of RGB565 pixels (contiguous, row by row) to the screen
 * at (x, y). Blocks until sent. A rectangle that is not fully on screen is
 * rejected with a logged error. */
void display_push(const uint16_t *pixels, int x, int y, int w, int h);
