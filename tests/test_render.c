#include <stdlib.h>

#include "assets.h"
#include "display.h"
#include "render.h"
#include "tank_helpers.h"
#include "test.h"

#define PELLET_COLOUR RGB565(214, 170, 96)

static uint16_t at(const uint16_t *fb, int x, int y)
{
    return fb[y * DISPLAY_W + x];
}

static void test_pellet_is_drawn(void)
{
    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    tank_t t;
    tank_init(&t, 1);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 300, 190); /* out of the way */
    }

    render_tank(&t, NOON, fb);
    CHECK(at(fb, 50, 30) == asset_water); /* open water, above the art */

    t.pellets[3] = (pellet_t){.alive = true, .x = 50, .y = 30};
    render_tank(&t, NOON, fb);
    CHECK(at(fb, 50, 30) == PELLET_COLOUR);
    CHECK(at(fb, 49, 29) == PELLET_COLOUR && at(fb, 51, 31) == PELLET_COLOUR); /* 3x3 */
    CHECK(at(fb, 52, 30) == asset_water);

    t.pellets[3].alive = false;
    render_tank(&t, NOON, fb);
    CHECK(at(fb, 50, 30) == asset_water);

    /* A pellet at the very edge must not write outside the buffer. */
    t.pellets[0] = (pellet_t){.alive = true, .x = 0, .y = 0};
    t.pellets[1] = (pellet_t){.alive = true, .x = 319, .y = 239};
    render_tank(&t, NOON, fb);
    CHECK(at(fb, 0, 0) == PELLET_COLOUR);
    free(fb);
}

void test_render(void)
{
    test_pellet_is_drawn();
}
