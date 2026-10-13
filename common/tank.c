#include "tank.h"

#include <math.h>
#include <string.h>

#include "tuning.h"

#define TICK_S (TICK_MS / 1000.0f)

static const species_t CAST[TANK_FISH] = {
    SPECIES_CLOWNFISH, SPECIES_GOLDFISH, SPECIES_PUFFERFISH, SPECIES_CATFISH, SPECIES_ANGELFISH,
};

/* xorshift32: a tiny generator that turns one number into the next. It must
 * never be given 0, which it would repeat forever. */
static uint32_t rng_next(tank_t *t)
{
    uint32_t x = t->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    t->rng = x;
    return x;
}

static float rng_range(tank_t *t, float lo, float hi)
{
    return lo + (hi - lo) * (float)(rng_next(t) >> 8) / 16777216.0f;
}

/* A whole number from lo to hi inclusive. */
static int rng_int(tank_t *t, int lo, int hi)
{
    return lo + (int)(rng_next(t) % (uint32_t)(hi - lo + 1));
}

static int clamp_int(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void depth_band(depth_t depth, float *lo, float *hi)
{
    switch (depth) {
    case DEPTH_UPPER:
        *lo = DEPTH_UPPER_MIN;
        *hi = DEPTH_UPPER_MAX;
        break;
    case DEPTH_BOTTOM:
        *lo = DEPTH_BOTTOM_MIN;
        *hi = DEPTH_BOTTOM_MAX;
        break;
    case DEPTH_MIDDLE:
    default:
        *lo = DEPTH_MIDDLE_MIN;
        *hi = DEPTH_MIDDLE_MAX;
        break;
    }
}

static void pick_wander_target(tank_t *t, fish_t *f)
{
    float lo, hi;
    depth_band(species_info(f->species)->depth, &lo, &hi);
    f->tx = rng_range(t, SWIM_X_MIN, SWIM_X_MAX);
    f->ty = rng_range(t, lo, hi);
    f->target_ticks = rng_int(t, WANDER_MIN_TICKS, WANDER_MAX_TICKS);
}

void tank_init(tank_t *t, uint32_t seed)
{
    /* Zero everything, padding included, so two tanks can be compared byte for byte. */
    memset(t, 0, sizeof(*t));
    t->rng = seed != 0 ? seed : 0x9E3779B9u;
    for (int i = 0; i < 8; i++) {
        rng_next(t); /* stir, so nearby seeds do not start out alike */
    }

    for (int i = 0; i < TANK_FISH; i++) {
        fish_t *f = &t->fish[i];
        const species_info_t *info = species_info(CAST[i]);
        f->species = CAST[i];
        f->bold = clamp_int(info->bold + rng_int(t, -1, 1), 0, 9);
        f->social = clamp_int(info->social + rng_int(t, -1, 1), 0, 9);
        f->x = rng_range(t, SWIM_X_MIN, SWIM_X_MAX);
        f->y = rng_range(t, SWIM_Y_MIN, SWIM_Y_MAX);
        f->facing_left = rng_int(t, 0, 1) == 1;
        pick_wander_target(t, f);
    }
}

static void wander(tank_t *t, fish_t *f)
{
    float dx = f->tx - f->x;
    float dy = f->ty - f->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < WANDER_ARRIVE_PX || f->target_ticks <= 0) {
        pick_wander_target(t, f);
        dx = f->tx - f->x;
        dy = f->ty - f->y;
        dist = sqrtf(dx * dx + dy * dy);
    } else {
        f->target_ticks--;
    }

    float want_vx = 0.0f, want_vy = 0.0f;
    if (dist > 0.001f) {
        want_vx = dx / dist * WANDER_SPEED;
        want_vy = dy / dist * WANDER_SPEED;
    }
    f->vx += (want_vx - f->vx) * STEER_GAIN;
    f->vy += (want_vy - f->vy) * STEER_GAIN;
}

static void move(fish_t *f)
{
    f->x += f->vx * TICK_S;
    f->y += f->vy * TICK_S;

    if (f->x < SWIM_X_MIN) {
        f->x = SWIM_X_MIN;
        f->vx = 0.0f;
    } else if (f->x > SWIM_X_MAX) {
        f->x = SWIM_X_MAX;
        f->vx = 0.0f;
    }
    if (f->y < SWIM_Y_MIN) {
        f->y = SWIM_Y_MIN;
        f->vy = 0.0f;
    } else if (f->y > SWIM_Y_MAX) {
        f->y = SWIM_Y_MAX;
        f->vy = 0.0f;
    }

    /* Only turn the sprite round once it is clearly heading the other way. */
    if (f->vx < -FACING_FLIP_SPEED) {
        f->facing_left = true;
    } else if (f->vx > FACING_FLIP_SPEED) {
        f->facing_left = false;
    }
}

void tank_tick(tank_t *t, int minutes)
{
    (void)minutes; /* day and night arrive in Part 2 */
    t->tick++;
    for (int i = 0; i < TANK_FISH; i++) {
        wander(t, &t->fish[i]);
        move(&t->fish[i]);
    }
}
