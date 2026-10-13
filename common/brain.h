#pragma once

#include <stdbool.h>

#include "species.h"

/* What a fish can decide to do. The body in tank.c carries it out. */
typedef enum {
    GOAL_WANDER,
    GOAL_SEEK_FOOD,
    GOAL_REST,
    GOAL_SCHOOL,
    GOAL_INVESTIGATE,
    GOAL_FLEE,
    GOAL_HIDE,
    GOAL_COUNT
} goal_t;

/* How it goes about it. The body in tank.c shows it. The order is schema v1's. */
typedef enum {
    MOOD_CALM,
    MOOD_EAGER,
    MOOD_WARY,
    MOOD_PLAYFUL,
    MOOD_SLEEPY,
    MOOD_SULKY,
    MOOD_COUNT
} mood_t;

/* A brain's whole answer. */
typedef struct {
    goal_t goal;
    mood_t mood;
} choice_t;

/* How far away something is, roughly. */
typedef enum { DIST_NONE, DIST_NEAR, DIST_MID, DIST_FAR } dist_t;

/* Everything the brain is allowed to know about one fish's situation. */
typedef struct {
    species_t species;          /* which fish this is */
    int hunger, energy, stress; /* 0..9 */
    int bold, social;           /* 0..9 */
    bool night;
    bool startled;              /* a finger has just landed close by */
    goal_t current;             /* what it is doing now */
    dist_t food, friend_, finger, cover;
} percept_t;

/* The hand-written rules. Pure: the same percept always gives the same
 * answer. They know nothing of moods, so the mood is always calm. */
choice_t brain_choose(const percept_t *p);

/* Lower-case names for printing; "?" for a value that is not in the list. */
const char *goal_name(goal_t g);
const char *mood_name(mood_t m);
const char *dist_name(dist_t d);
