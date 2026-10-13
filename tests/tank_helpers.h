#pragma once

/* Shared by the tank test files. */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "tank.h"
#include "tuning.h"

#define DAY false
#define NIGHT true

static inline bool inside(const fish_t *f)
{
    return isfinite(f->x) && isfinite(f->y) && isfinite(f->vx) && isfinite(f->vy) &&
           f->x >= SWIM_X_MIN && f->x <= SWIM_X_MAX &&
           f->y >= SWIM_Y_MIN && f->y <= SWIM_Y_MAX;
}

static inline bool need_ok(float v)
{
    return isfinite(v) && v >= 0.0f && v <= NEED_MAX;
}

static inline bool needs_ok(const fish_t *f)
{
    return need_ok(f->hunger) && need_ok(f->energy) && need_ok(f->stress);
}

static inline bool all_ok(const tank_t *t)
{
    for (int i = 0; i < TANK_FISH; i++) {
        if (!inside(&t->fish[i]) || !needs_ok(&t->fish[i])) {
            return false;
        }
    }
    return true;
}

static inline void run(tank_t *t, int ticks)
{
    for (int i = 0; i < ticks; i++) {
        tank_tick(t, DAY);
    }
}

static inline int pellet_count(const tank_t *t)
{
    int n = 0;
    for (int i = 0; i < TANK_PELLETS; i++) {
        n += t->pellets[i].alive;
    }
    return n;
}

/* A quick tap: two ticks long. */
static inline void tap(tank_t *t, int x, int y)
{
    tank_set_finger(t, true, x, y);
    tank_tick(t, DAY);
    tank_set_finger(t, false, x, y);
    tank_tick(t, DAY);
}

/* Park every fish somewhere, still, so a test controls the geometry. */
static inline void place(tank_t *t, int fish, float x, float y)
{
    t->fish[fish].x = x;
    t->fish[fish].y = y;
    t->fish[fish].vx = 0.0f;
    t->fish[fish].vy = 0.0f;
}

/* Brains that always give the same answer, for testing one body at a time. */
static inline choice_t always_wander(const percept_t *p) { (void)p; return (choice_t){GOAL_WANDER, MOOD_CALM}; }
static inline choice_t always_seek_food(const percept_t *p) { (void)p; return (choice_t){GOAL_SEEK_FOOD, MOOD_CALM}; }
static inline choice_t always_rest(const percept_t *p) { (void)p; return (choice_t){GOAL_REST, MOOD_CALM}; }
static inline choice_t always_school(const percept_t *p) { (void)p; return (choice_t){GOAL_SCHOOL, MOOD_CALM}; }
static inline choice_t always_investigate(const percept_t *p) { (void)p; return (choice_t){GOAL_INVESTIGATE, MOOD_CALM}; }
static inline choice_t always_flee(const percept_t *p) { (void)p; return (choice_t){GOAL_FLEE, MOOD_CALM}; }
static inline choice_t always_hide(const percept_t *p) { (void)p; return (choice_t){GOAL_HIDE, MOOD_CALM}; }

/* A brain that gives whatever goal and mood a test last asked for. */
static goal_t forced_goal;
static mood_t forced_mood;
static inline choice_t forced(const percept_t *p) { (void)p; return (choice_t){forced_goal, forced_mood}; }
static inline void force(tank_t *t, goal_t g, mood_t m)
{
    forced_goal = g;
    forced_mood = m;
    tank_set_brain(t, forced);
}

/* A repeatable stream of presses, drags and releases all over the screen. */
typedef struct {
    uint32_t s;
    bool down;
    int x, y;
} script_t;

static inline void script_step(script_t *sc, tank_t *t)
{
    sc->s = sc->s * 1664525u + 1013904223u;
    uint32_t r = sc->s >> 8;
    if (r % 25 == 0) {
        sc->down = !sc->down;
        sc->x = (int)((r / 25) % 320);
        sc->y = (int)((r / 8000) % 240);
    } else if (sc->down && r % 3 == 0) {
        sc->x = (sc->x + 7) % 320;
    }
    tank_set_finger(t, sc->down, sc->x, sc->y);
}
