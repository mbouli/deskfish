#include "species.h"

#include <stddef.h>

static const species_info_t table[SPECIES_COUNT] = {
    [SPECIES_ANGELFISH] = {"angelfish", 5, 4, DEPTH_UPPER, 30},
    [SPECIES_BASS] = {"bass", 7, 2, DEPTH_MIDDLE, 60},
    [SPECIES_CATFISH] = {"catfish", 4, 3, DEPTH_BOTTOM, 180},
    [SPECIES_CLOWNFISH] = {"clownfish", 8, 6, DEPTH_MIDDLE, 10},
    [SPECIES_GOLDFISH] = {"goldfish", 6, 8, DEPTH_MIDDLE, 3},
    [SPECIES_PUFFERFISH] = {"pufferfish", 2, 3, DEPTH_MIDDLE, 120},
    [SPECIES_SURGEONFISH] = {"surgeonfish", 6, 7, DEPTH_UPPER, 20},
};

const species_info_t *species_info(species_t s)
{
    if ((int)s < 0 || (int)s >= SPECIES_COUNT) {
        return NULL;
    }
    return &table[s];
}
