#include "brain.h"

/* The hand-written stand-in for the model. First matching rule wins. */

#define SHY_BOLD_MAX 3          /* at or below this, a close finger means flee */
#define HIDE_STRESS_MIN 6
#define HUNGRY_MIN 5
#define CURIOUS_BOLD_MIN 6
#define CURIOUS_STRESS_MAX 4
#define TIRED_ENERGY_MAX 2      /* start resting at or below this */
#define RESTED_ENERGY_MIN 6     /* stop resting at or above this */
#define SOCIAL_MIN 5

goal_t brain_choose(const percept_t *p)
{
    if (p->finger == DIST_NEAR && (p->startled || p->bold <= SHY_BOLD_MAX)) {
        return GOAL_FLEE;
    }
    if (p->stress >= HIDE_STRESS_MIN && p->cover != DIST_NONE) {
        return GOAL_HIDE;
    }
    if (p->hunger >= HUNGRY_MIN && p->food != DIST_NONE) {
        return GOAL_SEEK_FOOD;
    }
    if (p->finger != DIST_NONE && p->bold >= CURIOUS_BOLD_MIN && p->stress <= CURIOUS_STRESS_MAX) {
        return GOAL_INVESTIGATE;
    }
    if (p->night || p->energy <= TIRED_ENERGY_MAX ||
        (p->current == GOAL_REST && p->energy < RESTED_ENERGY_MIN)) {
        return GOAL_REST;
    }
    if (p->social >= SOCIAL_MIN && p->friend_ != DIST_NONE) {
        return GOAL_SCHOOL;
    }
    return GOAL_WANDER;
}

const char *goal_name(goal_t g)
{
    static const char *names[GOAL_COUNT] = {
        [GOAL_WANDER] = "wander",
        [GOAL_SEEK_FOOD] = "seek_food",
        [GOAL_REST] = "rest",
        [GOAL_SCHOOL] = "school",
        [GOAL_INVESTIGATE] = "investigate",
        [GOAL_FLEE] = "flee",
        [GOAL_HIDE] = "hide",
    };
    if ((int)g < 0 || (int)g >= GOAL_COUNT) {
        return "?";
    }
    return names[g];
}

const char *dist_name(dist_t d)
{
    switch (d) {
    case DIST_NONE:
        return "none";
    case DIST_NEAR:
        return "near";
    case DIST_MID:
        return "mid";
    case DIST_FAR:
        return "far";
    default:
        return "?";
    }
}
