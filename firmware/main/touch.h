#pragma once

#include <stdbool.h>

/* Start the touch chip. Returns false if it does not answer. */
bool touch_init(void);

/* True while a finger is down, with its position in screen pixels
 * (0 <= x < DISPLAY_W, 0 <= y < DISPLAY_H). False if touch never started. */
bool touch_read(int *x, int *y);
