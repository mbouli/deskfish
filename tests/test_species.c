#include <string.h>

#include "species.h"
#include "test.h"

static void check(species_t s, const char *name, int bold, int social, depth_t depth, int hunger_minutes)
{
    const species_info_t *info = species_info(s);
    CHECK(info != NULL);
    if (info == NULL) {
        return;
    }
    CHECK(strcmp(info->name, name) == 0);
    CHECK(info->bold == bold);
    CHECK(info->social == social);
    CHECK(info->depth == depth);
    CHECK(info->hunger_minutes == hunger_minutes);
}

void test_species(void)
{
    CHECK(SPECIES_COUNT == 7);
    check(SPECIES_ANGELFISH, "angelfish", 5, 4, DEPTH_UPPER, 30);
    check(SPECIES_BASS, "bass", 7, 2, DEPTH_MIDDLE, 60);
    check(SPECIES_CATFISH, "catfish", 4, 3, DEPTH_BOTTOM, 180);
    check(SPECIES_CLOWNFISH, "clownfish", 8, 6, DEPTH_MIDDLE, 10);
    check(SPECIES_GOLDFISH, "goldfish", 6, 8, DEPTH_MIDDLE, 3);
    check(SPECIES_PUFFERFISH, "pufferfish", 2, 3, DEPTH_MIDDLE, 120);
    check(SPECIES_SURGEONFISH, "surgeonfish", 6, 7, DEPTH_UPPER, 20);

    CHECK(species_info(SPECIES_COUNT) == NULL);
    CHECK(species_info((species_t)-1) == NULL);
}
