#include <math.h>

#include "assets.h"
#include "tank.h"
#include "tank_helpers.h"
#include "test.h"
#include "tuning.h"

/* True if no foreground art is under the fish's own 32x32 box. */
static bool sprite_clear_of_art(const fish_t *f)
{
    int c0 = (int)floorf((f->x - 16.0f) / ASSET_MASK_CELL), c1 = (int)floorf((f->x + 15.0f) / ASSET_MASK_CELL);
    int r0 = (int)floorf((f->y - 16.0f) / ASSET_MASK_CELL), r1 = (int)floorf((f->y + 15.0f) / ASSET_MASK_CELL);
    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            if (r >= 0 && r < ASSET_MASK_ROWS && c >= 0 && c < ASSET_MASK_COLS && asset_front_mask[r][c]) {
                return false;
            }
        }
    }
    return true;
}

static void test_fish_start_on_either_layer(void)
{
    int front = 0, back = 0;
    for (uint32_t seed = 1; seed <= 40; seed++) {
        tank_t t;
        tank_init(&t, seed);
        for (int f = 0; f < TANK_FISH; f++) {
            front += t.fish[f].front;
            back += !t.fish[f].front;
            CHECK(t.fish[f].depth == (t.fish[f].front ? 1.0f : 0.0f)); /* no size change at start */
        }
    }
    CHECK(front > 40 && back > 40); /* roughly half and half over 200 fish */
}

static void test_layer_changes_only_in_open_water(void)
{
    int flips = 0, flips_over_art = 0;
    for (uint32_t seed = 1; seed <= 4; seed++) {
        tank_t t;
        tank_init(&t, seed);
        tank_set_brain(&t, always_wander);
        bool was_front[TANK_FISH];
        for (int f = 0; f < TANK_FISH; f++) {
            was_front[f] = t.fish[f].front;
        }
        for (int i = 0; i < 20000; i++) {
            tank_tick(&t, NOON);
            for (int f = 0; f < TANK_FISH; f++) {
                if (t.fish[f].front != was_front[f]) {
                    flips++;
                    flips_over_art += !sprite_clear_of_art(&t.fish[f]);
                    was_front[f] = t.fish[f].front;
                }
            }
        }
    }
    CHECK(flips > 20);           /* it does happen */
    CHECK(flips_over_art == 0);  /* and never where it would pop through a rock */
}

static void test_depth_eases(void)
{
    tank_t t;
    tank_init(&t, 2);
    tank_set_brain(&t, always_wander);
    bool ok = true, saw_in_between = false;
    float last[TANK_FISH];
    for (int f = 0; f < TANK_FISH; f++) {
        last[f] = t.fish[f].depth;
    }
    for (int i = 0; i < 20000; i++) {
        tank_tick(&t, NOON);
        for (int f = 0; f < TANK_FISH; f++) {
            float d = t.fish[f].depth;
            ok = ok && d >= 0.0f && d <= 1.0f && fabsf(d - last[f]) <= 0.2f; /* no jumps in size */
            saw_in_between = saw_in_between || (d > 0.2f && d < 0.8f);
            last[f] = d;
        }
    }
    CHECK(ok);
    CHECK(saw_in_between);
}

static void test_hiding_fish_go_to_the_back(void)
{
    tank_t t;
    tank_init(&t, 3);
    tank_set_brain(&t, always_hide);
    run(&t, 600);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(!t.fish[f].front);
        CHECK(t.fish[f].depth == 0.0f);
    }
}

static void test_fish_in_front_of_a_rock_still_hides(void)
{
    tank_t t;
    tank_init(&t, 3);
    tank_set_brain(&t, always_hide);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, tank_cover[1][0], tank_cover[1][1]); /* deep in the big rock: never clear */
        t.fish[f].front = true;
        t.fish[f].want_front = true;
        t.fish[f].depth = 1.0f;
    }
    run(&t, 5);
    CHECK(t.fish[0].front); /* not instantly... */
    run(&t, LAYER_FORCE_TICKS + DECIDE_TICKS);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(!t.fish[f].front); /* ...but it does not stay "hidden" in plain view */
    }
}

static void test_layers_do_not_change_behaviour(void)
{
    /* A front fish and a back fish still see each other and still eat. */
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_seek_food);
    place(&t, 0, 100, 60);
    place(&t, 1, 130, 60);
    for (int f = 2; f < TANK_FISH; f++) {
        place(&t, f, 300, 30.0f + 40.0f * (float)f);
    }
    t.fish[0].front = t.fish[0].want_front = true;
    t.fish[1].front = t.fish[1].want_front = false;
    percept_t p;
    tank_percept(&t, 0, &p);
    CHECK(p.friend_ == DIST_NEAR);

    t.fish[0].hunger = t.fish[1].hunger = 8.0f;
    tap(&t, 100, 60);
    tap(&t, 130, 60);
    run(&t, 60);
    CHECK(t.fish[0].hunger < 6.0f);
    CHECK(t.fish[1].hunger < 6.0f);
}

void test_tank_depth(void)
{
    test_fish_start_on_either_layer();
    test_layer_changes_only_in_open_water();
    test_depth_eases();
    test_hiding_fish_go_to_the_back();
    test_fish_in_front_of_a_rock_still_hides();
    test_layers_do_not_change_behaviour();
}
