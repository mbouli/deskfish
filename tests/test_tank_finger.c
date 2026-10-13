#include <math.h>

#include "tank.h"
#include "tank_helpers.h"
#include "test.h"
#include "tuning.h"

static float finger_distance(const tank_t *t, int f)
{
    return hypotf(t->fish[f].x - t->touch_x, t->fish[f].y - t->touch_y);
}

/* ---- Startle ----------------------------------------------------------- */

static int startled_calls;

static goal_t notice_startle(const percept_t *p)
{
    startled_calls += p->startled;
    return GOAL_REST;
}

static void test_startle(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, notice_startle);
    place(&t, 0, 100, 100); /* 10 px from the touch */
    place(&t, 1, 130, 100); /* 30 px: inside the startle radius */
    place(&t, 2, 160, 100); /* 50 px: outside it */
    place(&t, 3, 300, 190);
    place(&t, 4, 300, 30);
    t.fish[0].bold = 2;
    t.fish[1].bold = 9;
    t.fish[2].bold = 2;
    for (int f = 0; f < TANK_FISH; f++) {
        t.fish[f].stress = 0.0f;
    }
    run(&t, 1);
    startled_calls = 0;

    tank_set_finger(&t, true, 110, 100);
    run(&t, 1);
    CHECK(fabsf(t.fish[0].stress - 7.0f) < 0.1f); /* 9 - bold */
    CHECK(fabsf(t.fish[1].stress - 1.0f) < 0.1f); /* never less than 1 */
    CHECK(t.fish[2].stress < 0.5f);               /* too far away to be startled */
    CHECK(startled_calls == 2);                   /* both were asked that same tick */
    CHECK(!t.fish[0].startled);                   /* and the flag was cleared */

    run(&t, 5);                                   /* holding still startles no one again */
    CHECK(startled_calls == 2);

    t.fish[0].stress = 8.0f;                      /* stress cannot go past the top */
    tank_set_finger(&t, false, 110, 100);
    run(&t, TAP_MAX_TICKS + 2);
    place(&t, 0, 100, 100);
    tank_set_finger(&t, true, 110, 100);
    run(&t, 1);
    CHECK(t.fish[0].stress <= NEED_MAX);
}

static void test_shy_fish_get_stressed_near_a_finger(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_rest);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 300, 30 + f * 10); /* everyone far away... */
    }
    place(&t, 0, 100, 185);             /* ...except these two, already at resting depth */
    place(&t, 1, 100, 195);
    t.fish[0].bold = 2;
    t.fish[1].bold = 8;
    tank_set_finger(&t, true, 160, 190); /* 60 px away: no startle */
    run(&t, 1);
    t.fish[0].stress = 0.0f;
    t.fish[1].stress = 0.0f;

    tank_set_finger(&t, true, 120, 190); /* slide in close: still no startle */
    run(&t, 40);
    CHECK(t.fish[0].stress > 1.0f);  /* the shy one is getting worked up */
    CHECK(t.fish[1].stress == 0.0f); /* the bold one does not care */
}

/* ---- Flee -------------------------------------------------------------- */

static void test_flee(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_flee);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 150 + f * 5, 110);
    }
    tank_set_finger(&t, true, 160, 110);
    run(&t, 1);
    float before[TANK_FISH];
    for (int f = 0; f < TANK_FISH; f++) {
        before[f] = finger_distance(&t, f);
    }
    run(&t, 40); /* two seconds */
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(finger_distance(&t, f) > before[f] + 40.0f);
    }
    CHECK(all_ok(&t));
}

static void test_flee_from_zero_distance(void)
{
    tank_t t;
    tank_init(&t, 1);
    tank_set_brain(&t, always_flee);
    place(&t, 0, 160, 110);
    tank_set_finger(&t, true, 160, 110); /* exactly on the fish */
    run(&t, 40);
    CHECK(all_ok(&t));
    CHECK(finger_distance(&t, 0) > 30.0f); /* it picked a direction and went */
}

static void test_flee_without_a_finger_is_wander(void)
{
    tank_t t;
    tank_init(&t, 7);
    tank_set_brain(&t, always_flee);
    run(&t, 1200);
    CHECK(t.fish[0].goal == GOAL_FLEE);
    CHECK(all_ok(&t));
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(hypotf(t.fish[f].vx, t.fish[f].vy) < FLEE_SPEED * 0.6f); /* not at flee speed */
    }
}

/* ---- Investigate ------------------------------------------------------- */

static void test_investigate(void)
{
    tank_t t;
    tank_init(&t, 2);
    tank_set_brain(&t, always_investigate);
    tank_set_finger(&t, true, 160, 120);
    run(&t, 400);
    for (int f = 0; f < TANK_FISH; f++) {
        float d = finger_distance(&t, f);
        CHECK(d < INVESTIGATE_GAP_PX + 20.0f); /* it came over */
        CHECK(d > INVESTIGATE_GAP_PX - 20.0f); /* but kept a respectful distance */
    }
    /* And it keeps moving round the finger instead of parking. */
    float x0 = t.fish[0].x, y0 = t.fish[0].y;
    run(&t, 60);
    CHECK(hypotf(t.fish[0].x - x0, t.fish[0].y - y0) > 8.0f);
    CHECK(all_ok(&t));
}

/* ---- Hide -------------------------------------------------------------- */

static void test_hide(void)
{
    tank_t t;
    tank_init(&t, 3);
    tank_set_brain(&t, always_hide);
    run(&t, 600);
    for (int f = 0; f < TANK_FISH; f++) {
        float best = 1e9f;
        for (int c = 0; c < TANK_COVERS; c++) {
            float d = hypotf(t.fish[f].x - tank_cover[c][0], t.fish[f].y - tank_cover[c][1]);
            best = d < best ? d : best;
        }
        CHECK(best < HIDE_ARRIVE_PX + 3.0f);              /* tucked in at a cover point */
        CHECK(hypotf(t.fish[f].vx, t.fish[f].vy) < 3.0f); /* and staying put */
    }
}

static void test_cover_points_are_reachable(void)
{
    for (int c = 0; c < TANK_COVERS; c++) {
        CHECK(tank_cover[c][0] >= SWIM_X_MIN && tank_cover[c][0] <= SWIM_X_MAX);
        CHECK(tank_cover[c][1] >= SWIM_Y_MIN && tank_cover[c][1] <= SWIM_Y_MAX);
    }
}

/* ---- The whole thing --------------------------------------------------- */

static void test_shy_and_bold_react_differently(void)
{
    tank_t t;
    tank_init(&t, 1); /* the real rules */
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 300, 30 + f * 10);
        t.fish[f].hunger = 0.0f;
        t.fish[f].energy = 9.0f;
        t.fish[f].stress = 0.0f;
    }
    place(&t, 0, 100, 100);
    place(&t, 1, 105, 110);
    t.fish[0].bold = 2; /* shy */
    t.fish[1].bold = 8; /* bold */
    tank_set_finger(&t, true, 110, 105);
    run(&t, 1);
    CHECK(t.fish[0].goal == GOAL_FLEE);
    CHECK(t.fish[1].goal == GOAL_FLEE); /* even the bold one jumps at first */

    run(&t, DECIDE_TICKS * 3);
    CHECK(t.fish[0].goal == GOAL_FLEE || t.fish[0].goal == GOAL_HIDE);
    CHECK(t.fish[1].goal == GOAL_INVESTIGATE); /* then curiosity wins */
}

static void test_every_goal_stays_inside(void)
{
    static const brain_fn brains[GOAL_COUNT] = {always_wander, always_seek_food, always_rest, always_school,
                                                always_investigate, always_flee, always_hide};
    for (int g = 0; g < GOAL_COUNT; g++) {
        tank_t t;
        script_t sc = {.s = 1000u + (uint32_t)g};
        tank_init(&t, 50u + (uint32_t)g);
        tank_set_brain(&t, brains[g]);
        bool ok = true;
        for (int i = 0; i < 6000 && ok; i++) {
            script_step(&sc, &t);
            tank_tick(&t, DAY);
            ok = all_ok(&t);
        }
        CHECK(ok);
    }
}

void test_tank_finger(void)
{
    test_startle();
    test_shy_fish_get_stressed_near_a_finger();
    test_flee();
    test_flee_from_zero_distance();
    test_flee_without_a_finger_is_wander();
    test_investigate();
    test_hide();
    test_cover_points_are_reachable();
    test_shy_and_bold_react_differently();
    test_every_goal_stays_inside();
}
