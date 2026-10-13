#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sky.h"

/* Turns a sky into what to draw. No hardware and no pictures. */

#define LIGHT_BANDS 8      /* flat bands of sky colour around the sun */
#define LIGHT_HALOS 3      /* rings around the moon */
#define LIGHT_MAX_STARS 46

typedef struct {
    /* Band i covers everything closer to the centre than band_radius[i] and
     * outside band i-1. The last band has no radius: it is the rest of the sky. */
    int band_radius[LIGHT_BANDS - 1];
    uint16_t band_colour[LIGHT_BANDS];
    int centre_x, centre_y; /* what the bands ring: the sun, or where it set */

    bool show_sun;          /* drawn at centre_x, centre_y */
    bool show_moon;
    int moon_x, moon_y;
    float moon_phase;       /* as in sky_t */
    bool show_halo;         /* rings around the moon, replacing the outer band's colour */
    uint16_t halo_colour[LIGHT_HALOS];

    int star_count;         /* how many of the fixed stars to draw, 0..LIGHT_MAX_STARS */
    float tint[3];          /* red, green, blue multipliers for sand, foreground and fish */
    bool fish_night;        /* dark enough that fish should rest */
} light_t;

void light_compute(const sky_t *sky, light_t *out);
