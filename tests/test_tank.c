#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "tank.h"
#include "tank_helpers.h"
#include "test.h"
#include "tuning.h"

/* ---- Carried over from Part 1 ----------------------------------------- */

static void test_cast(void)
{
    static const species_t cast[TANK_FISH] = {SPECIES_CLOWNFISH, SPECIES_GOLDFISH, SPECIES_PUFFERFISH,
                                              SPECIES_CATFISH, SPECIES_ANGELFISH};
    for (uint32_t seed = 1; seed <= 50; seed++) {
        tank_t t;
        tank_init(&t, seed);
        CHECK(t.tick == 0);
        CHECK(pellet_count(&t) == 0);
        CHECK(!t.touching);
        for (int i = 0; i < TANK_FISH; i++) {
            const fish_t *f = &t.fish[i];
            const species_info_t *info = species_info(f->species);
            CHECK(f->species == cast[i]);
            CHECK(inside(f));
            CHECK(needs_ok(f));
            CHECK(f->goal == GOAL_WANDER);
            CHECK(f->bold >= 0 && f->bold <= 9 && abs(f->bold - info->bold) <= 1);
            CHECK(f->social >= 0 && f->social <= 9 && abs(f->social - info->social) <= 1);
        }
    }
}

static void test_same_seed_same_run(void)
{
    tank_t a, b, c;
    script_t sa = {.s = 9}, sb = {.s = 9}, sc = {.s = 9};
    tank_init(&a, 1234);
    tank_init(&b, 1234);
    tank_init(&c, 1235);
    for (int i = 0; i < 3000; i++) {
        script_step(&sa, &a);
        script_step(&sb, &b);
        script_step(&sc, &c);
        tank_tick(&a, NOON);
        tank_tick(&b, NOON);
        tank_tick(&c, NOON);
    }
    CHECK(a.tick == 3000);
    CHECK(memcmp(&a, &b, sizeof(tank_t)) == 0);
    CHECK(memcmp(&a, &c, sizeof(tank_t)) != 0);
}

static void test_stays_inside_with_input(void)
{
    for (uint32_t seed = 1; seed <= 5; seed++) {
        tank_t t;
        script_t sc = {.s = seed * 77};
        tank_init(&t, seed);
        bool ok = true;
        for (int i = 0; i < 20000 && ok; i++) {
            script_step(&sc, &t);
            tank_tick(&t, i < 10000 ? NOON : 0); /* half by day, half by night */
            ok = all_ok(&t);
        }
        CHECK(ok);
    }
}

static void test_fish_move(void)
{
    tank_t t;
    tank_init(&t, 7);
    tank_set_brain(&t, always_wander);
    float start_x[TANK_FISH], start_y[TANK_FISH], furthest[TANK_FISH] = {0};
    for (int f = 0; f < TANK_FISH; f++) {
        start_x[f] = t.fish[f].x;
        start_y[f] = t.fish[f].y;
    }
    for (int i = 0; i < 1200; i++) {
        tank_tick(&t, NOON);
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
    CHECK(all_ok(&t));
    CHECK(t.fish[0].x != x0 || t.fish[0].y != y0);
    CHECK(t.fish[0].x != t.fish[1].x || t.fish[0].y != t.fish[1].y);
}

static void test_depth_preference(void)
{
    tank_t t;
    tank_init(&t, 99);
    tank_set_brain(&t, always_wander);
    run(&t, 800);
    bool catfish_low = true, angelfish_high = true;
    for (int i = 0; i < 6000; i++) {
        tank_tick(&t, NOON);
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
    tank_set_brain(&t, always_wander);
    int flips[TANK_FISH] = {0};
    int back_to_back = 0;
    bool was_left[TANK_FISH], flipped_last[TANK_FISH] = {false};
    for (int f = 0; f < TANK_FISH; f++) {
        was_left[f] = t.fish[f].facing_left;
    }
    for (int i = 0; i < 12000; i++) {
        tank_tick(&t, NOON);
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
        CHECK(flips[f] < 200);
    }
}

/* ---- Needs ------------------------------------------------------------- */

static void test_needs_move_the_right_way(void)
{
    tank_t t;
    tank_init(&t, 3);
    tank_set_brain(&t, always_wander);
    for (int f = 0; f < TANK_FISH; f++) {
        t.fish[f].hunger = 4.0f;
        t.fish[f].energy = 5.0f;
        t.fish[f].stress = 5.0f;
    }
    run(&t, 100);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].hunger > 4.0f); /* gets hungrier */
        CHECK(t.fish[f].energy < 5.0f); /* tires while active */
        CHECK(t.fish[f].stress < 5.0f); /* calms down */
    }

    tank_set_brain(&t, always_rest);
    run(&t, 60); /* long enough for every fish to be asked */
    float before = t.fish[0].energy;
    run(&t, 100);
    CHECK(t.fish[0].energy > before); /* recovers while resting */
}

static void test_needs_stay_in_range(void)
{
    tank_t t;
    tank_init(&t, 4);
    tank_set_brain(&t, always_wander);
    bool ok = true;
    for (int i = 0; i < 20000 && ok; i++) { /* long enough to hit both ends */
        tank_tick(&t, NOON);
        ok = all_ok(&t);
    }
    CHECK(ok);
    CHECK(t.fish[0].hunger == NEED_MAX);
    CHECK(t.fish[0].energy == 0.0f);

    tank_set_brain(&t, always_rest);
    for (int i = 0; i < 5000 && ok; i++) {
        tank_tick(&t, NOON);
        ok = all_ok(&t);
    }
    CHECK(ok);
    CHECK(t.fish[0].energy == NEED_MAX);
}

static void test_appetite_depends_on_species(void)
{
    for (uint32_t seed = 1; seed <= 20; seed++) {
        tank_t t;
        tank_init(&t, seed);
        for (int f = 0; f < TANK_FISH; f++) {
            float ticks = (float)species_info(t.fish[f].species)->hunger_minutes * 1200.0f;
            float typical = NEED_MAX / ticks;
            CHECK(t.fish[f].hunger_per_tick > typical * 0.79f);
            CHECK(t.fish[f].hunger_per_tick < typical * 1.21f);
        }
    }

    tank_t t;
    tank_init(&t, 3);
    tank_set_brain(&t, always_wander);
    for (int f = 0; f < TANK_FISH; f++) {
        t.fish[f].hunger = 0.0f;
    }
    run(&t, 1200); /* one minute */
    const fish_t *goldfish = &t.fish[1], *catfish = &t.fish[3];
    CHECK(goldfish->species == SPECIES_GOLDFISH && catfish->species == SPECIES_CATFISH);
    CHECK(goldfish->hunger > 2.0f);                    /* a third of the way to starving already */
    CHECK(catfish->hunger < 0.1f);                     /* barely peckish */
    CHECK(goldfish->hunger > catfish->hunger * 10.0f);
}

/* ---- Day and night ----------------------------------------------------- */

static void test_night_flag(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_tick(&t, 0);
    CHECK(t.night);
    tank_tick(&t, DAY_START_MIN - 1);
    CHECK(t.night);
    tank_tick(&t, DAY_START_MIN);
    CHECK(!t.night);
    tank_tick(&t, NIGHT_START_MIN - 1);
    CHECK(!t.night);
    tank_tick(&t, NIGHT_START_MIN);
    CHECK(t.night);
    tank_tick(&t, 1439);
    CHECK(t.night);
}

/* ---- Taps and pellets -------------------------------------------------- */

static void test_tap_drops_one_pellet(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);
    tank_set_finger(&t, true, 100, 100);
    run(&t, 3);
    CHECK(t.touching);
    CHECK(pellet_count(&t) == 0); /* nothing until the finger lifts */
    tank_set_finger(&t, false, 100, 100);
    run(&t, 1);
    CHECK(!t.touching);
    CHECK(pellet_count(&t) == 1);
    CHECK(fabsf(t.pellets[0].x - 100.0f) < 0.01f);
    CHECK(t.pellets[0].y >= 100.0f);
}

static void test_fast_tap_between_ticks(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);
    tank_set_finger(&t, true, 150, 80);
    tank_set_finger(&t, false, 150, 80); /* down and up before any tick */
    run(&t, 1);
    CHECK(pellet_count(&t) == 1);
    run(&t, 5);
    CHECK(pellet_count(&t) == 1);

    /* Down, up, down again before a tick: one tap, then a fresh touch. */
    tank_set_finger(&t, true, 200, 80);
    tank_set_finger(&t, false, 200, 80);
    tank_set_finger(&t, true, 60, 80);
    run(&t, 2);
    CHECK(pellet_count(&t) == 2);
    CHECK(t.touching);
    tank_set_finger(&t, false, 60, 80);
    run(&t, 1);
    CHECK(pellet_count(&t) == 3);
}

static void test_hold_and_drag_drop_nothing(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);

    tank_set_finger(&t, true, 100, 100);
    run(&t, TAP_MAX_TICKS + 1); /* too long */
    tank_set_finger(&t, false, 100, 100);
    run(&t, 1);
    CHECK(pellet_count(&t) == 0);

    tank_set_finger(&t, true, 100, 100);
    run(&t, 1);
    tank_set_finger(&t, true, 130, 100); /* moved too far */
    run(&t, 1);
    tank_set_finger(&t, false, 130, 100);
    run(&t, 1);
    CHECK(pellet_count(&t) == 0);

    tap(&t, 100, 100); /* an exact-limit tap still counts */
    CHECK(pellet_count(&t) == 1);
}

static void test_ninth_pellet_refused(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);
    for (int i = 0; i < 12; i++) {
        tap(&t, 30 + i * 20, 60);
    }
    CHECK(pellet_count(&t) == TANK_PELLETS);
}

static void test_tap_at_screen_corners(void)
{
    static const int corners[4][2] = {{0, 0}, {319, 0}, {0, 239}, {319, 239}};
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);
    for (int i = 0; i < 4; i++) {
        tap(&t, corners[i][0], corners[i][1]);
    }
    CHECK(pellet_count(&t) == 4);
    for (int i = 0; i < TANK_PELLETS; i++) {
        if (t.pellets[i].alive) {
            CHECK(t.pellets[i].x >= SWIM_X_MIN && t.pellets[i].x <= SWIM_X_MAX);
            CHECK(t.pellets[i].y >= SWIM_Y_MIN && t.pellets[i].y <= PELLET_FLOOR_Y);
        }
    }
}

static void test_pellets_sink_settle_and_expire(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_wander);
    tap(&t, 100, 60);
    float y0 = t.pellets[0].y;
    run(&t, 20);
    CHECK(t.pellets[0].y > y0);
    CHECK(!t.pellets[0].settled);

    run(&t, 400); /* (206 - 60) / 15 px/s is under 10 seconds */
    CHECK(t.pellets[0].alive && t.pellets[0].settled);
    CHECK(t.pellets[0].y == PELLET_FLOOR_Y);

    run(&t, PELLET_LIFE_TICKS - 10);
    CHECK(pellet_count(&t) == 0);
}

/* ---- Eating ------------------------------------------------------------ */

static void test_hungry_fish_reaches_food_and_eats(void)
{
    tank_t t;
    tank_init(&t, 11);
    tank_set_brain(&t, always_seek_food);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 30.0f + 30.0f * (float)f, 40.0f); /* well away from where the food lands */
        t.fish[f].hunger = 8.0f;
    }
    tap(&t, 250, 120);
    CHECK(pellet_count(&t) == 1);

    bool eaten = false, hunger_fell = false;
    for (int i = 0; i < 400 && !eaten; i++) {
        float before[TANK_FISH];
        for (int f = 0; f < TANK_FISH; f++) {
            before[f] = t.fish[f].hunger;
        }
        tank_tick(&t, NOON);
        if (pellet_count(&t) == 0) {
            eaten = true;
            for (int f = 0; f < TANK_FISH; f++) {
                hunger_fell = hunger_fell || t.fish[f].hunger < before[f] - 3.0f;
            }
        }
    }
    CHECK(eaten);
    CHECK(hunger_fell);
}

static void test_only_seekers_eat(void)
{
    tank_t t;
    tank_init(&t, 11);
    tank_set_brain(&t, always_wander);
    run(&t, 40);
    /* Drop a settled pellet right on top of a wandering fish. */
    t.pellets[0] = (pellet_t){.alive = true, .settled = true, .x = t.fish[0].x, .y = t.fish[0].y};
    run(&t, 1);
    CHECK(pellet_count(&t) == 1);
}

static void test_goal_without_target_is_wander(void)
{
    tank_t t;
    tank_init(&t, 7);
    tank_set_brain(&t, always_seek_food); /* but there is no food */
    float x0 = t.fish[2].x, y0 = t.fish[2].y, furthest = 0.0f;
    for (int i = 0; i < 1200; i++) {
        tank_tick(&t, NOON);
        float d = hypotf(t.fish[2].x - x0, t.fish[2].y - y0);
        furthest = d > furthest ? d : furthest;
    }
    CHECK(furthest > 30.0f);                    /* it wandered */
    CHECK(t.fish[2].goal == GOAL_SEEK_FOOD);    /* and the goal was left alone */
    CHECK(all_ok(&t));
}

static void test_rest_is_low_and_slow(void)
{
    tank_t t;
    tank_init(&t, 8);
    tank_set_brain(&t, always_rest);
    run(&t, 3000);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].y > REST_Y - 8.0f);
        CHECK(hypotf(t.fish[f].vx, t.fish[f].vy) < 2.0f);
    }
}

/* ---- Percepts ---------------------------------------------------------- */

static void test_percept(void)
{
    tank_t t;
    tank_init(&t, 1);
    place(&t, 0, 100, 100);
    place(&t, 1, 130, 100);                 /* 30 px: near */
    place(&t, 2, 300, 30);
    place(&t, 3, 300, 60);
    place(&t, 4, 300, 190);
    t.pellets[0] = (pellet_t){.alive = true, .x = 200, .y = 100}; /* 100 px: mid */
    t.fish[0].hunger = 4.6f;
    t.fish[0].energy = 0.4f;
    t.fish[0].stress = 8.9f;
    t.fish[0].bold = 7;
    t.fish[0].social = 2;
    t.fish[0].goal = GOAL_REST;
    t.fish[0].startled = true;
    t.night = true;

    percept_t p;
    tank_percept(&t, 0, &p);
    CHECK(p.hunger == 5 && p.energy == 0 && p.stress == 9); /* rounded */
    CHECK(p.bold == 7 && p.social == 2);
    CHECK(p.night && p.startled);
    CHECK(p.current == GOAL_REST);
    CHECK(p.food == DIST_MID);
    CHECK(p.friend_ == DIST_NEAR);
    CHECK(p.finger == DIST_NONE);
    CHECK(p.cover == DIST_NEAR); /* (126,102) is 26 px away */

    tank_percept(&t, 4, &p);     /* far from everything except the statue's cover */
    CHECK(p.food == DIST_FAR);
    CHECK(p.friend_ == DIST_FAR);

    t.pellets[0].alive = false;
    tank_percept(&t, 0, &p);
    CHECK(p.food == DIST_NONE);

    tank_percept(&t, -1, &p);    /* nonsense index: a harmless empty percept */
    CHECK(p.food == DIST_NONE && p.current == GOAL_WANDER);
    tank_percept(&t, TANK_FISH, &p);
    CHECK(p.food == DIST_NONE);
}

static void test_finger_is_seen_while_down(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_rest); /* keeps fish from wandering off mid-test */
    place(&t, 0, 100, 100);
    percept_t p;

    tank_set_finger(&t, true, 120, 100);
    run(&t, 1);
    tank_percept(&t, 0, &p);
    CHECK(p.finger == DIST_NEAR);

    tank_set_finger(&t, true, 190, 100); /* the presence follows the finger */
    run(&t, 1);
    place(&t, 0, 100, 100);
    tank_percept(&t, 0, &p);
    CHECK(p.finger == DIST_MID);

    tank_set_finger(&t, false, 190, 100);
    run(&t, 1);
    tank_percept(&t, 0, &p);
    CHECK(p.finger == DIST_NONE);
}

/* ---- Decisions --------------------------------------------------------- */

static int calls, calls_seeing_food;

static goal_t counting(const percept_t *p)
{
    calls++;
    calls_seeing_food += p->food != DIST_NONE;
    return GOAL_REST; /* resting fish drift little, so sightings stay put */
}

static void test_fish_are_asked_in_turn_and_regularly(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, counting);
    /* Park the fish in a row at resting depth, 65 px apart, so they hold
     * still and no sighting changes bucket during the test. */
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 30.0f + 65.0f * (float)f, REST_Y);
        t.fish[f].goal = GOAL_REST; /* resting from the start, not whenever first asked */
    }
    run(&t, 1); /* lets every fish register its new surroundings */
    calls = 0;

    run(&t, DECIDE_TICKS);
    CHECK(calls == TANK_FISH);       /* each asked exactly once per period */
    run(&t, DECIDE_TICKS * 3);
    CHECK(calls == TANK_FISH * 4);

    /* And not all on the same tick. */
    int busiest = 0;
    for (int i = 0; i < DECIDE_TICKS; i++) {
        int before = calls;
        run(&t, 1);
        busiest = calls - before > busiest ? calls - before : busiest;
    }
    CHECK(busiest == 1);
}

static void test_new_sighting_forces_a_decision(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, counting);
    run(&t, 2);
    calls_seeing_food = 0;
    tap(&t, 160, 100); /* food appears: every fish's food sighting changes */
    run(&t, 1);
    CHECK(calls_seeing_food >= TANK_FISH);
}

static goal_t nonsense(const percept_t *p)
{
    (void)p;
    return (goal_t)42;
}

static void test_bad_goal_is_ignored(void)
{
    tank_t t;
    tank_init(&t, 6);
    tank_set_brain(&t, nonsense);
    run(&t, 600);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].goal == GOAL_WANDER); /* kept what it had */
    }
    CHECK(all_ok(&t));

    tank_set_brain(&t, NULL); /* NULL restores the built-in rules */
    CHECK(t.choose == brain_choose);
}

void test_tank(void)
{
    test_cast();
    test_same_seed_same_run();
    test_stays_inside_with_input();
    test_fish_move();
    test_seed_zero_works();
    test_depth_preference();
    test_no_facing_flicker();
    test_needs_move_the_right_way();
    test_needs_stay_in_range();
    test_appetite_depends_on_species();
    test_night_flag();
    test_tap_drops_one_pellet();
    test_fast_tap_between_ticks();
    test_hold_and_drag_drop_nothing();
    test_ninth_pellet_refused();
    test_tap_at_screen_corners();
    test_pellets_sink_settle_and_expire();
    test_hungry_fish_reaches_food_and_eats();
    test_only_seekers_eat();
    test_goal_without_target_is_wander();
    test_rest_is_low_and_slow();
    test_percept();
    test_finger_is_seen_while_down();
    test_fish_are_asked_in_turn_and_regularly();
    test_new_sighting_forces_a_decision();
    test_bad_goal_is_ignored();
}
