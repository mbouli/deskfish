#pragma once

/* In alphabetical order, matching the sprite file names in assets/fish/. */
typedef enum {
    SPECIES_ANGELFISH,
    SPECIES_BASS,
    SPECIES_CATFISH,
    SPECIES_CLOWNFISH,
    SPECIES_GOLDFISH,
    SPECIES_PUFFERFISH,
    SPECIES_SURGEONFISH,
    SPECIES_COUNT
} species_t;

typedef enum { DEPTH_UPPER, DEPTH_MIDDLE, DEPTH_BOTTOM } depth_t;

typedef struct {
    const char *name;
    int bold;      /* 0..9 */
    int social;    /* 0..9 */
    depth_t depth; /* where it likes to swim */
} species_info_t;

/* NULL if s is not a species. */
const species_info_t *species_info(species_t s);
