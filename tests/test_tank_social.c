#include <math.h>

#include "tank.h"
#include "tank_helpers.h"
#include "test.h"
#include "tuning.h"

static float gap(const tank_t *t, int a, int b)
{
    return hypotf(t->fish[a].x - t->fish[b].x, t->fish[a].y - t->fish[b].y);
}

static float nearest_gap(const tank_t *t, int f)
{
    float best = 1e9f;
    for (int o = 0; o < TANK_FISH; o++) {
        if (o != f && gap(t, f, o) < best) {
            best = gap(t, f, o);
        }
    }
    return best;
}

static void test_school_gathers(void)
{
    tank_t t;
    tank_init(&t, 4);
    tank_set_brain(&t, always_school);
    place(&t, 0, 30, 40);
    place(&t, 1, 290, 40);
    place(&t, 2, 30, 190);
    place(&t, 3, 290, 190);
    place(&t, 4, 160, 110);
    run(&t, 1200);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(nearest_gap(&t, f) < SCHOOL_GAP_PX + SCHOOL_BAND_PX + 15.0f);
    }
    CHECK(all_ok(&t));
}

static void test_school_keeps_moving(void)
{
    tank_t t;
    tank_init(&t, 4);
    tank_set_brain(&t, always_school);
    run(&t, 1200);
    float x0[TANK_FISH], y0[TANK_FISH], furthest = 0.0f;
    for (int f = 0; f < TANK_FISH; f++) {
        x0[f] = t.fish[f].x;
        y0[f] = t.fish[f].y;
    }
    for (int i = 0; i < 1200; i++) {
        tank_tick(&t, DAY);
        for (int f = 0; f < TANK_FISH; f++) {
            float d = hypotf(t.fish[f].x - x0[f], t.fish[f].y - y0[f]);
            furthest = d > furthest ? d : furthest;
        }
    }
    CHECK(furthest > 30.0f); /* a school that froze in place is not a school */
}

static void test_fish_keep_apart(void)
{
    tank_t t;
    tank_init(&t, 12);
    tank_set_brain(&t, always_wander);
    int crowded = 0;
    const int ticks = 20000;
    for (int i = 0; i < ticks; i++) {
        tank_tick(&t, DAY);
        bool close = false;
        for (int a = 0; a < TANK_FISH; a++) {
            for (int b = a + 1; b < TANK_FISH; b++) {
                close = close || gap(&t, a, b) < 8.0f;
            }
        }
        crowded += close;
    }
    CHECK(crowded < ticks / 50); /* on top of each other under 2% of the time */
}

static void test_separation_at_zero_distance(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_rest);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 160, 188); /* all five on the same spot, at resting depth */
    }
    run(&t, 200);
    CHECK(all_ok(&t));
    for (int a = 0; a < TANK_FISH; a++) {
        for (int b = a + 1; b < TANK_FISH; b++) {
            CHECK(gap(&t, a, b) > 6.0f); /* they spread out */
        }
    }
}

static void test_night_costs_more_energy(void)
{
    tank_t day, night;
    tank_init(&day, 9);
    tank_init(&night, 9);
    tank_set_brain(&day, always_wander);
    tank_set_brain(&night, always_wander);
    day.fish[0].energy = night.fish[0].energy = 8.0f;
    for (int i = 0; i < 200; i++) {
        tank_tick(&day, DAY);
        tank_tick(&night, NIGHT);
    }
    float day_cost = 8.0f - day.fish[0].energy;
    float night_cost = 8.0f - night.fish[0].energy;
    CHECK(day_cost > 0.0f);
    CHECK(fabsf(night_cost - day_cost * ENERGY_NIGHT_FACTOR) < 0.01f);
}

static void test_fish_sleep_at_night_and_wake(void)
{
    tank_t t;
    tank_init(&t, 21); /* the real rules */
    for (int f = 0; f < TANK_FISH; f++) {
        t.fish[f].hunger = 0.0f;
        t.fish[f].energy = 9.0f;
    }
    for (int i = 0; i < 400; i++) {
        tank_tick(&t, NIGHT);
    }
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].goal == GOAL_REST);
    }
    for (int i = 0; i < DECIDE_TICKS * 2; i++) {
        tank_tick(&t, DAY);
    }
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].goal != GOAL_REST); /* fully rested, so up at dawn */
    }
}

void test_tank_social(void)
{
    test_school_gathers();
    test_school_keeps_moving();
    test_fish_keep_apart();
    test_separation_at_zero_distance();
    test_night_costs_more_energy();
    test_fish_sleep_at_night_and_wake();
}
