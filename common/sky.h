#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float lat_deg; /* north is positive */
    float lon_deg; /* east is positive */
} place_t;

typedef struct {
    float sun_alt_deg; /* height above the horizon; negative when below */
    bool sun_up;       /* above -0.833 degrees, the usual definition of sunrise */
    float progress;    /* 0..1 through the day if sun_up, otherwise through the night */
    float moon_phase;  /* 0 new, 0.25 first quarter, 0.5 full, 0.75 last quarter */
} sky_t;

/* unix_utc is seconds since 1970-01-01 00:00 UTC. */
void sky_compute(int64_t unix_utc, place_t place, sky_t *out);

/* Seconds since 1970 for a calendar date and time in UTC. */
int64_t sky_unix_from_utc(int year, int month, int day, int hour, int minute);
