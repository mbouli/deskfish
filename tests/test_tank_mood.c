#include <math.h>
#include <string.h>

#include "tank_helpers.h"
#include "test.h"

static choice_t bad_mood(const percept_t *p) { (void)p; return (choice_t){GOAL_REST, (mood_t)17}; }
static choice_t bad_goal(const percept_t *p) { (void)p; return (choice_t){(goal_t)-1, MOOD_EAGER}; }

static void test_mood_reaches_the_fish(void)
{
    tank_t t;
    tank_init(&t, 5);
    for (int i = 0; i < TANK_FISH; i++) {
        CHECK(t.fish[i].mood == MOOD_CALM);
        CHECK(t.fish[i].sign_ticks == 0);
    }
    force(&t, GOAL_WANDER, MOOD_PLAYFUL);
    run(&t, DECIDE_TICKS + DECIDE_STAGGER_TICKS * TANK_FISH);
    for (int i = 0; i < TANK_FISH; i++) {
        CHECK(t.fish[i].mood == MOOD_PLAYFUL);
    }
}

static void test_a_bad_answer_changes_nothing(void)
{
    tank_t t;
    tank_init(&t, 5);
    force(&t, GOAL_WANDER, MOOD_WARY);
    run(&t, 80);
    tank_set_brain(&t, bad_mood);
    run(&t, 80);
    for (int i = 0; i < TANK_FISH; i++) {
        CHECK(t.fish[i].goal == GOAL_WANDER && t.fish[i].mood == MOOD_WARY);
    }
    tank_set_brain(&t, bad_goal);
    run(&t, 80);
    for (int i = 0; i < TANK_FISH; i++) {
        CHECK(t.fish[i].goal == GOAL_WANDER && t.fish[i].mood == MOOD_WARY);
    }
}

static void test_sign_timing(void)
{
    tank_t t;
    tank_init(&t, 5);
    force(&t, GOAL_WANDER, MOOD_SLEEPY);
    int shown = 0, ticks = 0;
    while (t.fish[0].mood != MOOD_SLEEPY && ticks++ < 200) {
        tank_tick(&t, DAY);
    }
    CHECK(t.fish[0].sign_ticks == SIGN_TICKS); /* set on the tick the mood changed */
    while (t.fish[0].sign_ticks > 0 && shown < 200) {
        tank_tick(&t, DAY);
        shown++;
    }
    CHECK(shown == SIGN_TICKS);
    run(&t, 100);
    CHECK(t.fish[0].sign_ticks == 0); /* the same mood again does not show it again */

    force(&t, GOAL_WANDER, MOOD_EAGER);
    run(&t, 60);
    force(&t, GOAL_WANDER, MOOD_CALM); /* back to calm: the sign goes at once, and none appears */
    ticks = 0;
    while (t.fish[0].mood != MOOD_CALM && ticks++ < 200) {
        tank_tick(&t, DAY);
    }
    CHECK(t.fish[0].sign_ticks == 0);
}

static void test_mood_names(void)
{
    static const char *names[MOOD_COUNT] = {"calm", "eager", "wary", "playful", "sleepy", "sulky"};
    for (int m = 0; m < MOOD_COUNT; m++) {
        CHECK(strcmp(mood_name((mood_t)m), names[m]) == 0);
    }
    CHECK(strcmp(mood_name(MOOD_COUNT), "?") == 0);
    CHECK(strcmp(mood_name((mood_t)-1), "?") == 0);
}

/* How far fish 0 travels in a straight wander, alone, in a mood. */
static float mean_speed(goal_t goal, mood_t mood)
{
    tank_t t;
    tank_init(&t, 11);
    force(&t, goal, mood);
    for (int i = 1; i < TANK_FISH; i++) {
        place(&t, i, 20.0f + 8.0f * (float)i, 190.0f); /* out of the way */
    }
    float total = 0.0f;
    int n = 0;
    for (int tick = 0; tick < 600; tick++) {
        for (int i = 1; i < TANK_FISH; i++) {
            place(&t, i, 20.0f + 8.0f * (float)i, 190.0f);
        }
        if (goal == GOAL_FLEE) { /* keep a finger on it so it keeps fleeing */
            tank_set_finger(&t, true, (int)t.fish[0].x + 20, (int)t.fish[0].y);
            if (tick % 40 == 0) {
                place(&t, 0, 160.0f, 110.0f);
            }
        }
        tank_tick(&t, DAY);
        if (tick >= 100) {
            total += sqrtf(t.fish[0].vx * t.fish[0].vx + t.fish[0].vy * t.fish[0].vy);
            n++;
        }
    }
    return total / (float)n;
}

static void test_moods_change_speed(void)
{
    float calm = mean_speed(GOAL_WANDER, MOOD_CALM);
    CHECK(mean_speed(GOAL_WANDER, MOOD_EAGER) > calm * 1.15f);
    CHECK(mean_speed(GOAL_WANDER, MOOD_WARY) < calm * 0.9f);
    CHECK(mean_speed(GOAL_WANDER, MOOD_SLEEPY) < calm * 0.75f);
    CHECK(mean_speed(GOAL_WANDER, MOOD_SULKY) < calm * 0.85f);
}

static void test_flee_is_the_same_in_every_mood(void)
{
    float calm = mean_speed(GOAL_FLEE, MOOD_CALM);
    for (int m = 1; m < MOOD_COUNT; m++) {
        float v = mean_speed(GOAL_FLEE, (mood_t)m);
        CHECK(fabsf(v - calm) < 0.001f);
    }
}

/* Fish 0's mean speed during the burst part of each two seconds, over its
 * mean speed during the rest. */
static float burst_ratio(mood_t mood)
{
    tank_t t;
    tank_init(&t, 11);
    force(&t, GOAL_WANDER, mood);
    float burst = 0.0f, coast = 0.0f;
    int bursts = 0, coasts = 0;
    for (int tick = 0; tick < 1200; tick++) {
        for (int i = 1; i < TANK_FISH; i++) {
            place(&t, i, 20.0f + 8.0f * (float)i, 190.0f);
        }
        tank_tick(&t, DAY);
        if (tick < 200) {
            continue;
        }
        float v = sqrtf(t.fish[0].vx * t.fish[0].vx + t.fish[0].vy * t.fish[0].vy);
        /* Late in each part, once its speed has caught up with what it wants. */
        uint32_t phase = t.tick % PLAYFUL_PERIOD_TICKS;
        if (phase >= PLAYFUL_BURST_TICKS / 2 && phase < PLAYFUL_BURST_TICKS) {
            burst += v;
            bursts++;
        } else if (phase >= (PLAYFUL_PERIOD_TICKS + PLAYFUL_BURST_TICKS) / 2) {
            coast += v;
            coasts++;
        }
    }
    CHECK(all_ok(&t));
    return (burst / (float)bursts) / (coast / (float)coasts);
}

static void test_playful_comes_in_bursts(void)
{
    CHECK(burst_ratio(MOOD_PLAYFUL) > 1.2f);
    CHECK(burst_ratio(MOOD_CALM) < 1.1f); /* a calm fish keeps a steady pace */
}

/* The gap two still fish settle at, 30 px apart to start, in a mood. */
static float settled_gap(mood_t mood)
{
    tank_t t;
    tank_init(&t, 11);
    force(&t, GOAL_REST, mood);
    for (int i = 0; i < TANK_FISH; i++) {
        place(&t, i, 30.0f + 60.0f * (float)i, REST_Y);
    }
    place(&t, 1, 60.0f, REST_Y); /* fish 0 and 1 start 30 px apart */
    place(&t, 2, 280.0f, 40.0f);
    run(&t, 400);
    return fabsf(t.fish[1].x - t.fish[0].x);
}

static void test_wary_and_sulky_keep_their_distance(void)
{
    float calm = settled_gap(MOOD_CALM);
    CHECK(settled_gap(MOOD_WARY) > calm + 4.0f);
    CHECK(settled_gap(MOOD_SULKY) > settled_gap(MOOD_WARY));
}

static void test_a_hiding_fish_holds_its_spot_in_any_mood(void)
{
    for (int m = 0; m < MOOD_COUNT; m++) {
        tank_t t;
        tank_init(&t, 11);
        force(&t, GOAL_HIDE, (mood_t)m);
        for (int i = 0; i < TANK_FISH; i++) {
            place(&t, i, tank_cover[0][0] + 2.0f * (float)i, tank_cover[0][1]);
        }
        run(&t, 300);
        for (int i = 0; i < TANK_FISH; i++) {
            float dx = t.fish[i].x - tank_cover[0][0], dy = t.fish[i].y - tank_cover[0][1];
            CHECK(sqrtf(dx * dx + dy * dy) < HIDE_ARRIVE_PX + 4.0f);
        }
    }
}

/* Schooling fish copy each other's speed. If a mood sped up what was copied,
 * two eager fish would wind each other up without limit. */
static void test_a_school_in_a_mood_does_not_run_away(void)
{
    static const mood_t fast[] = {MOOD_EAGER, MOOD_PLAYFUL};
    for (int m = 0; m < 2; m++) {
        tank_t t;
        tank_init(&t, 3);
        force(&t, GOAL_SCHOOL, fast[m]);
        float top = 0.0f;
        for (int tick = 0; tick < 4000; tick++) {
            tank_tick(&t, DAY);
            for (int i = 0; tick > 400 && i < TANK_FISH; i++) {
                float v = sqrtf(t.fish[i].vx * t.fish[i].vx + t.fish[i].vy * t.fish[i].vy);
                top = v > top ? v : top;
            }
        }
        /* The schooling pace, as fast as the mood makes it, plus being pushed apart. */
        CHECK(top < SCHOOL_SPEED * PLAYFUL_BURST_SPEED + SEPARATION_PUSH);
        CHECK(all_ok(&t));
    }
}

void test_tank_mood(void)
{
    test_mood_reaches_the_fish();
    test_a_bad_answer_changes_nothing();
    test_sign_timing();
    test_mood_names();
    test_moods_change_speed();
    test_flee_is_the_same_in_every_mood();
    test_playful_comes_in_bursts();
    test_wary_and_sulky_keep_their_distance();
    test_a_hiding_fish_holds_its_spot_in_any_mood();
    test_a_school_in_a_mood_does_not_run_away();
}
