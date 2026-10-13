#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "species.h"
#include "tank.h"

/* Scene drawing. Call in this order: background, each fish, foreground. */

void render_background(uint16_t *fb);

/* Draw one fish centred on (cx, cy), size_px wide and tall. */
void render_fish(uint16_t *fb, species_t s, int cx, int cy, bool facing_left, int size_px);

void render_foreground(uint16_t *fb);

/* Draw the whole tank. */
void render_tank(const tank_t *t, uint16_t *fb);
