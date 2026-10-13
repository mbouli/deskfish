#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "species.h"

/* Scene drawing. Call in this order: background, each fish, foreground. */

void render_background(uint16_t *fb);

/* Draw one fish centred on (cx, cy). */
void render_fish(uint16_t *fb, species_t s, int cx, int cy, bool facing_left);

void render_foreground(uint16_t *fb);
