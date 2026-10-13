#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "tank.h"
#include "test.h"
#include "tuning.h"

static bool inside(const fish_t *f)
{
    return isfinite(f->x) && isfinite(f->y) &&
           f->x >= SWIM_X_MIN && f->x <= SWIM_X_MAX &&
           f->y >= SWIM_Y_MIN && f->y <= SWIM_Y_MAX;
}

static void run(tank_t *t, int ticks)
{
    for (int i = 0; i < ticks; i++) {
        tank_tick(t, 12 * 60);
    }
}

static void test_cast(void)
{
    static const species_t cast[TANK_FISH] = {SPECIES_CLOWNFISH, SPECIES_GOLDFISH, SPECIES_PUFFERFISH,
                                              SPECIES_CATFISH, SPECIES_ANGELFISH};
    for (uint32_t seed = 1; seed <= 50; seed++) {
        tank_t t;
        tank_init(&t, seed);
        CHECK(t.tick == 0);
        for (int i = 0; i < TANK_FISH; i++) {
            const fish_t *f = &t.fish[i];
            const species_info_t *info = species_info(f->species);
            CHECK(f->species == cast[i]);
            CHECK(inside(f));
            CHECK(f->bold >= 0 && f->bold <= 9 && abs(f->bold - info->bold) <= 1);
            CHECK(f->social >= 0 && f->social <= 9 && abs(f->social - info->social) <= 1);
        }
    }
}

static void test_same_seed_same_run(void)
{
    tank_t a, b, c;
    tank_init(&a, 1234);
    tank_init(&b, 1234);
    tank_init(&c, 1235);
    run(&a, 2000);
    run(&b, 2000);
    run(&c, 2000);
    CHECK(a.tick == 2000);
    CHECK(memcmp(&a, &b, sizeof(tank_t)) == 0);
    CHECK(memcmp(&a, &c, sizeof(tank_t)) != 0);
}

static void test_stays_inside(void)
{
    for (uint32_t seed = 1; seed <= 5; seed++) {
        tank_t t;
        tank_init(&t, seed);
        bool ok = true;
        for (int i = 0; i < 20000 && ok; i++) {
            tank_tick(&t, 12 * 60);
            for (int f = 0; f < TANK_FISH; f++) {
                ok = ok && inside(&t.fish[f]);
            }
        }
        CHECK(ok);
    }
}

static void test_fish_move(void)
{
    tank_t t;
    tank_init(&t, 7);
    float start_x[TANK_FISH], start_y[TANK_FISH];
    for (int f = 0; f < TANK_FISH; f++) {
        start_x[f] = t.fish[f].x;
        start_y[f] = t.fish[f].y;
    }
    float furthest[TANK_FISH] = {0};
    for (int i = 0; i < 1200; i++) { /* one minute */
        tank_tick(&t, 12 * 60);
        for (int f = 0; f < TANK_FISH; f++) {
            float d = hypotf(t.fish[f].x - start_x[f], t.fish[f].y - start_y[f]);
            if (d > furthest[f]) {
                furthest[f] = d;
            }
        }
    }
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(furthest[f] > 30.0f);
    }
}

static void test_seed_zero_works(void)
{
    tank_t t;
    tank_init(&t, 0);
    float x0 = t.fish[0].x, y0 = t.fish[0].y;
    CHECK(t.rng != 0);
    run(&t, 600);
    CHECK(inside(&t.fish[0]));
    CHECK(t.fish[0].x != x0 || t.fish[0].y != y0);
    /* The five fish must not all have been dealt the same position. */
    CHECK(t.fish[0].x != t.fish[1].x || t.fish[0].y != t.fish[1].y);
}

static void test_depth_preference(void)
{
    tank_t t;
    tank_init(&t, 99);
    run(&t, 800); /* time to reach their own depth */
    bool catfish_low = true, angelfish_high = true;
    for (int i = 0; i < 6000; i++) {
        tank_tick(&t, 12 * 60);
        for (int f = 0; f < TANK_FISH; f++) {
            if (t.fish[f].species == SPECIES_CATFISH && t.fish[f].y < DEPTH_BOTTOM_MIN - 6.0f) {
                catfish_low = false;
            }
            if (t.fish[f].species == SPECIES_ANGELFISH && t.fish[f].y > DEPTH_UPPER_MAX + 6.0f) {
                angelfish_high = false;
            }
        }
    }
    CHECK(catfish_low);
    CHECK(angelfish_high);
}

static void test_no_facing_flicker(void)
{
    tank_t t;
    tank_init(&t, 5);
    int flips[TANK_FISH] = {0};
    int back_to_back = 0;
    bool was_left[TANK_FISH], flipped_last[TANK_FISH] = {false};
    for (int f = 0; f < TANK_FISH; f++) {
        was_left[f] = t.fish[f].facing_left;
    }
    for (int i = 0; i < 12000; i++) { /* ten minutes */
        tank_tick(&t, 12 * 60);
        for (int f = 0; f < TANK_FISH; f++) {
            bool flipped = t.fish[f].facing_left != was_left[f];
            flips[f] += flipped;
            back_to_back += flipped && flipped_last[f];
            flipped_last[f] = flipped;
            was_left[f] = t.fish[f].facing_left;
        }
    }
    CHECK(back_to_back == 0);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(flips[f] < 200); /* a turn every few seconds, not a strobe */
    }
}

void test_tank(void)
{
    test_cast();
    test_same_seed_same_run();
    test_stays_inside();
    test_fish_move();
    test_seed_zero_works();
    test_depth_preference();
    test_no_facing_flicker();
}
