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

    render_tank(&t, fb);
    CHECK(at(fb, 50, 30) == asset_water); /* open water, above the art */

    t.pellets[3] = (pellet_t){.alive = true, .x = 50, .y = 30};
    render_tank(&t, fb);
    CHECK(at(fb, 50, 30) == PELLET_COLOUR);
    CHECK(at(fb, 49, 29) == PELLET_COLOUR && at(fb, 51, 31) == PELLET_COLOUR); /* 3x3 */
    CHECK(at(fb, 52, 30) == asset_water);

    t.pellets[3].alive = false;
    render_tank(&t, fb);
    CHECK(at(fb, 50, 30) == asset_water);

    /* A pellet at the very edge must not write outside the buffer. */
    t.pellets[0] = (pellet_t){.alive = true, .x = 0, .y = 0};
    t.pellets[1] = (pellet_t){.alive = true, .x = 319, .y = 239};
    render_tank(&t, fb);
    CHECK(at(fb, 0, 0) == PELLET_COLOUR);
    free(fb);
}

/* The box around every pixel that differs between two frames. */
static void changed_box(const uint16_t *a, const uint16_t *b, int *x0, int *x1, int *n)
{
    *x0 = DISPLAY_W;
    *x1 = -1;
    *n = 0;
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            if (at(a, x, y) != at(b, x, y)) {
                *x0 = x < *x0 ? x : *x0;
                *x1 = x > *x1 ? x : *x1;
                (*n)++;
            }
        }
    }
}

static void test_layers_and_sizes(void)
{
    uint16_t *empty = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    tank_t t;
    tank_init(&t, 1);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 300, 30); /* stack them all in one corner */
        t.fish[f].front = false;
        t.fish[f].depth = 0.0f;
    }
    render_tank(&t, empty);
    int x0, x1, n;

    /* Deep inside the big rock: a back fish is completely covered... */
    place(&t, 0, tank_cover[1][0], tank_cover[1][1]);
    render_tank(&t, fb);
    changed_box(empty, fb, &x0, &x1, &n);
    CHECK(n == 0);
    /* ...and a front fish is drawn over the rock. */
    t.fish[0].front = true;
    t.fish[0].depth = 1.0f;
    render_tank(&t, fb);
    changed_box(empty, fb, &x0, &x1, &n);
    CHECK(n > 100);

    /* In open water, a back fish is drawn a little smaller than a front one. */
    place(&t, 0, 100, 60);
    t.fish[0].front = false;
    t.fish[0].depth = 0.0f;
    render_tank(&t, fb);
    changed_box(empty, fb, &x0, &x1, &n);
    int back_width = x1 - x0 + 1;
    t.fish[0].front = true;
    t.fish[0].depth = 1.0f;
    render_tank(&t, fb);
    changed_box(empty, fb, &x0, &x1, &n);
    int front_width = x1 - x0 + 1;
    CHECK(front_width <= FISH_FRONT_PX && back_width <= FISH_BACK_PX);
    CHECK(front_width > back_width);
    CHECK(front_width - back_width <= 6); /* slight, not dramatic */

    free(empty);
    free(fb);
}

void test_render(void)
{
    test_layers_and_sizes();
    test_pellet_is_drawn();
}
