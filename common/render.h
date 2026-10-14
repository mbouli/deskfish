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

/* A mood sign is this many pixels wide and tall, not counting its outline. */
#define SIGN_PX 7

/* Where a mood sign's top-left pixel goes for a fish drawn centred on (cx, cy)
 * at size_px: over its head, its lowest drawn row resting on the fish's top
 * edge, and wholly on screen. */
void render_sign_place(species_t s, int cx, int cy, bool facing_left, int size_px, mood_t mood, int *x, int *y);

/* Draw a mood's sign with its top-left pixel at (x, y). Calm has none. */
void render_sign(uint16_t *fb, mood_t mood, int x, int y);

void render_tank(const tank_t *t, uint16_t *fb);
