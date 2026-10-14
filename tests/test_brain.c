#include <string.h>

#include "brain.h"
#include "test.h"

/* A fish with nothing going on: every rule falls through to wander. */
static percept_t calm(void)
{
    percept_t p = {
        .hunger = 0, .energy = 9, .stress = 0,
        .bold = 5, .social = 0,
        .night = false, .startled = false,
        .current = GOAL_WANDER,
        .food = DIST_NONE, .friend_ = DIST_NONE, .finger = DIST_NONE, .cover = DIST_NONE,
    };
    return p;
}

static void test_default_is_wander(void)
{
    percept_t p = calm();
    CHECK(brain_choose(&p).mood == MOOD_CALM); /* the rules have no moods */
    CHECK(brain_choose(&p).goal == GOAL_WANDER);
}

static void test_flee(void)
{
    percept_t p = calm();
    p.finger = DIST_NEAR;
    p.bold = 3;
    CHECK(brain_choose(&p).goal == GOAL_FLEE);
    p.bold = 4;
    CHECK(brain_choose(&p).goal != GOAL_FLEE);
    p.bold = 9;
    p.startled = true;
    CHECK(brain_choose(&p).goal == GOAL_FLEE); /* even a bold fish jumps when startled */
    p.finger = DIST_MID;
    CHECK(brain_choose(&p).goal != GOAL_FLEE); /* but only when the finger is close */
}

static void test_hide(void)
{
    percept_t p = calm();
    p.stress = 6;
    p.cover = DIST_FAR;
    CHECK(brain_choose(&p).goal == GOAL_HIDE);
    p.stress = 5;
    CHECK(brain_choose(&p).goal != GOAL_HIDE);
    p.stress = 9;
    p.cover = DIST_NONE;
    CHECK(brain_choose(&p).goal != GOAL_HIDE); /* nowhere to hide */
}

static void test_seek_food(void)
{
    percept_t p = calm();
    p.hunger = 5;
    p.food = DIST_FAR;
    CHECK(brain_choose(&p).goal == GOAL_SEEK_FOOD);
    p.hunger = 4;
    CHECK(brain_choose(&p).goal != GOAL_SEEK_FOOD);
    p.hunger = 9;
    p.food = DIST_NONE;
    CHECK(brain_choose(&p).goal != GOAL_SEEK_FOOD); /* hungry, but nothing to eat */
}

static void test_investigate(void)
{
    percept_t p = calm();
    p.finger = DIST_FAR;
    p.bold = 6;
    p.stress = 4;
    CHECK(brain_choose(&p).goal == GOAL_INVESTIGATE);
    p.bold = 5;
    CHECK(brain_choose(&p).goal != GOAL_INVESTIGATE);
    p.bold = 6;
    p.stress = 5;
    CHECK(brain_choose(&p).goal != GOAL_INVESTIGATE);
    p.stress = 0;
    p.finger = DIST_NONE;
    CHECK(brain_choose(&p).goal != GOAL_INVESTIGATE);
}

static void test_rest(void)
{
    percept_t p = calm();
    p.night = true;
    CHECK(brain_choose(&p).goal == GOAL_REST);
    p.night = false;
    p.energy = 2;
    CHECK(brain_choose(&p).goal == GOAL_REST);
    p.energy = 3;
    CHECK(brain_choose(&p).goal != GOAL_REST);

    /* Once resting, keep resting until properly recovered. */
    p.current = GOAL_REST;
    p.energy = 5;
    CHECK(brain_choose(&p).goal == GOAL_REST);
    p.energy = 6;
    CHECK(brain_choose(&p).goal != GOAL_REST);
}

static void test_school(void)
{
    percept_t p = calm();
    p.social = 5;
    p.friend_ = DIST_FAR;
    CHECK(brain_choose(&p).goal == GOAL_SCHOOL); /* a long way from anyone: go and join someone */
    p.social = 4;
    CHECK(brain_choose(&p).goal != GOAL_SCHOOL);
    p.social = 9;
    p.friend_ = DIST_NONE;
    CHECK(brain_choose(&p).goal != GOAL_SCHOOL);

    /* Still some way off: keep closing in... */
    p.friend_ = DIST_MID;
    CHECK(brain_choose(&p).goal == GOAL_SCHOOL);
    /* ...but once close, stop shadowing and do its own thing. */
    p.friend_ = DIST_NEAR;
    CHECK(brain_choose(&p).goal != GOAL_SCHOOL);
}

/* Start with nearly every rule matching, then knock them out from the top. */
static void test_priority_order(void)
{
    percept_t p = calm();
    p.startled = true;
    p.finger = DIST_NEAR;
    p.bold = 8;
    p.stress = 7;
    p.cover = DIST_NEAR;
    p.hunger = 9;
    p.food = DIST_NEAR;
    p.night = true;
    p.social = 9;
    p.friend_ = DIST_FAR;
    CHECK(brain_choose(&p).goal == GOAL_FLEE);

    p.startled = false;
    CHECK(brain_choose(&p).goal == GOAL_HIDE);

    p.stress = 4;
    CHECK(brain_choose(&p).goal == GOAL_SEEK_FOOD);

    p.food = DIST_NONE;
    CHECK(brain_choose(&p).goal == GOAL_INVESTIGATE);

    p.finger = DIST_NONE;
    CHECK(brain_choose(&p).goal == GOAL_REST);

    p.night = false;
    CHECK(brain_choose(&p).goal == GOAL_SCHOOL);

    p.friend_ = DIST_NONE;
    CHECK(brain_choose(&p).goal == GOAL_WANDER);
}

static void test_names(void)
{
    static const char *goals[GOAL_COUNT] = {"wander", "seek_food", "rest", "school",
                                            "investigate", "flee", "hide"};
    for (int g = 0; g < GOAL_COUNT; g++) {
        CHECK(strcmp(goal_name((goal_t)g), goals[g]) == 0);
    }
    CHECK(strcmp(goal_name(GOAL_COUNT), "?") == 0);
    CHECK(strcmp(goal_name((goal_t)-1), "?") == 0);

    CHECK(strcmp(dist_name(DIST_NONE), "none") == 0);
    CHECK(strcmp(dist_name(DIST_NEAR), "near") == 0);
    CHECK(strcmp(dist_name(DIST_MID), "mid") == 0);
    CHECK(strcmp(dist_name(DIST_FAR), "far") == 0);
    CHECK(strcmp(dist_name((dist_t)7), "?") == 0);
}

void test_brain(void)
{
    test_default_is_wander();
    test_flee();
    test_hide();
    test_seek_food();
    test_investigate();
    test_rest();
    test_school();
    test_priority_order();
    test_names();
}
