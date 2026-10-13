#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"
#include "light.h"
#include "species.h"

/* The pictures the tank is drawn from, lit for the current time of day.
 * Lighting is slow, so it is done here, once per change of light, and every
 * frame is then drawn from these copies. */

/* Allocate the lit pictures. Safe to call more than once. False if memory ran out. */
bool scene_init(void);

/* Rebuild every lit picture for this light. */
void scene_relight(const light_t *light);

const image_t *scene_backdrop(void);        /* DISPLAY_W x DISPLAY_H, opaque: sky, ruins, sand */
const image_t *scene_front(void);           /* DISPLAY_W x 192, keyed: rocks and kelp */
const image_t *scene_fish(species_t s);     /* 16x16, keyed; NULL if s is not a species */
uint16_t scene_pellet(void);

unsigned scene_relight_count(void);
