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

void test_tank_mood(void)
{
    test_mood_reaches_the_fish();
    test_a_bad_answer_changes_nothing();
    test_sign_timing();
    test_mood_names();
}
