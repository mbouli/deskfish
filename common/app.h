#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "light.h"
#include "sky.h"
#include "tank.h"

/* The per-frame loop body shared by the simulator and the board. */

/* Start, or restart, with a fresh tank. False if memory ran out. */
bool app_init(uint32_t seed);

/* Read the touch, advance the tank by however much real time has passed, draw
 * it into fb and push it to the display. fb is DISPLAY_W x DISPLAY_H. */
void app_frame(uint16_t *fb);

/* Run the tank at a multiple of real time. 1 is normal; below 1 is treated as 1. */
void app_set_speed(int multiplier);

/* The sky clock: real time plus an offset. A rate of 3600 runs the sky at an
 * hour per second while the fish carry on as normal. Below 1 is treated as 1. */
void app_set_sky_rate(int multiplier);
void app_skip_sky(int64_t seconds);
int64_t app_sky_time(void);

/* For inspection. */
const tank_t *app_tank(void);
const sky_t *app_sky(void);
const light_t *app_light(void);
unsigned app_relights(void); /* how many times the light has been recalculated */
