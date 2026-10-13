#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "species.h"

#define TANK_FISH 5
#define TICK_MS 50 /* one tick is 1/20 second */

typedef struct {
    float x, y;       /* centre, in screen pixels */
    float vx, vy;     /* pixels per second */
    float tx, ty;     /* the point it is wandering toward */
    int target_ticks; /* ticks left before it gives up on that point */
    species_t species;
    int bold, social; /* 0..9: the species' values, nudged per fish */
    bool facing_left;
} fish_t;

typedef struct {
    uint32_t rng;  /* the tank's own random generator state */
    uint32_t tick; /* ticks since tank_init */
    fish_t fish[TANK_FISH];
} tank_t;

/* Set up the starting cast. The same seed always gives the same tank. */
void tank_init(tank_t *t, uint32_t seed);

/* Advance the tank by one tick. minutes is the time of day, 0..1439. */
void tank_tick(tank_t *t, int minutes);
