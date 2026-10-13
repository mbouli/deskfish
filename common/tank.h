#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "brain.h"
#include "species.h"

#define TANK_FISH 5
#define TANK_PELLETS 8
#define TANK_COVERS 5
#define TICK_MS 50 /* one tick is 1/20 second */

/* The four things a fish keeps track of the distance to, in this order. */
enum { SEE_FOOD, SEE_FRIEND, SEE_FINGER, SEE_COVER, SEE_COUNT };

/* Anything that answers a percept with a goal and a mood: the rules or the model. */
typedef choice_t (*brain_fn)(const percept_t *p);

typedef struct {
    float x, y;       /* centre, in screen pixels */
    float vx, vy;     /* pixels per second */
    float tx, ty;     /* the point it is wandering toward */
    int target_ticks; /* ticks left before it gives up on that point */
    species_t species;
    int bold, social; /* 0..9: the species' values, nudged per fish */
    bool facing_left;

    bool front;      /* drawn in front of the rocks and kelp, not behind */
    bool want_front; /* the layer it would like to be on */
    float depth;     /* 0 = back, 1 = front; follows `front` smoothly, for drawn size */
    int layer_wait;  /* ticks a hiding fish has waited for open water */

    float hunger, energy, stress; /* 0..9, kept as fractions so they change smoothly */
    float hunger_per_tick;        /* appetite: its species' rate, nudged per fish */
    goal_t goal;
    mood_t mood;                  /* how it goes about its goal */
    int sign_ticks;               /* ticks its mood sign has left to show */
    int decide_ticks;             /* ticks until the brain is next asked */
    bool startled;                /* set by a startle, cleared by the next decision */
    dist_t seen[SEE_COUNT];       /* sightings as of the last tick */
} fish_t;

typedef struct {
    bool alive;
    bool settled;      /* resting on the sand */
    float x, y;
    int settled_ticks; /* how long it has been settled */
} pellet_t;

typedef struct {
    uint32_t rng;  /* the tank's own random generator state */
    uint32_t tick; /* ticks since tank_init */
    bool night;    /* as of the last tick */
    brain_fn choose;

    fish_t fish[TANK_FISH];
    pellet_t pellets[TANK_PELLETS];

    /* Latest touch state, as given to tank_set_finger. */
    bool finger_down;
    float finger_x, finger_y;
    bool press_pending, release_pending; /* edges not yet seen by a tick */
    float press_x, press_y;

    /* The touch as the tank understands it, updated each tick. */
    bool touching;
    float touch_x, touch_y; /* where the finger is now */
    float start_x, start_y; /* where it first landed */
    int touch_ticks;
    bool touch_moved;       /* it has strayed too far to count as a tap */
} tank_t;

/* Positions (centre x, y) behind rocks and kelp where a fish is hidden. */
extern const float tank_cover[TANK_COVERS][2];

/* Set up the starting cast. The same seed always gives the same tank. */
void tank_init(tank_t *t, uint32_t seed);

/* The same, with a cast of your choosing. An entry that is not a species, or
 * a NULL cast, uses the usual cast's fish for that place. */
void tank_init_cast(tank_t *t, uint32_t seed, const species_t cast[TANK_FISH]);

/* Use a different brain. NULL restores the built-in rules. */
void tank_set_brain(tank_t *t, brain_fn fn);

/* Report the latest touch state. Call as often as you like between ticks. */
void tank_set_finger(tank_t *t, bool down, int x, int y);

/* Advance the tank by one tick. night is whether it is dark enough to rest. */
void tank_tick(tank_t *t, bool night);

/* Describe one fish's situation the way its brain sees it. */
void tank_percept(const tank_t *t, int fish, percept_t *out);
