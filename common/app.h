#pragma once

#include <stdint.h>

/* The per-frame loop body shared by the simulator and the board. */

void app_init(uint32_t seed);

/* Read input, advance, draw into fb and push it to the display.
 * fb is DISPLAY_W x DISPLAY_H. */
void app_frame(uint16_t *fb);
