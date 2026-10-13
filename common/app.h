#pragma once

#include <stdint.h>

#include "tank.h"

/* The per-frame loop body shared by the simulator and the board. */

/* Start, or restart, with a fresh tank. */
void app_init(uint32_t seed);

/* Advance the tank by however much real time has passed, draw it into fb and
 * push it to the display. fb is DISPLAY_W x DISPLAY_H. */
void app_frame(uint16_t *fb);

/* Run the tank at a multiple of real time. 1 is normal; below 1 is treated as 1. */
void app_set_speed(int multiplier);

/* The tank, for inspection. */
const tank_t *app_tank(void);
