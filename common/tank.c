#include "tank.h"

#include <math.h>
#include <string.h>

#include "assets.h"
#include "tuning.h"

#define TICK_S (TICK_MS / 1000.0f)

static const species_t CAST[TANK_FISH] = {
    SPECIES_CLOWNFISH, SPECIES_GOLDFISH, SPECIES_PUFFERFISH, SPECIES_CATFISH, SPECIES_ANGELFISH,
};

/* Chosen where the foreground art covers most of a 32x32 fish. */
const float tank_cover[TANK_COVERS][2] = {
    {126.0f, 102.0f}, /* high on the big rock */
    {136.0f, 154.0f}, /* the big rock's base */
    {86.0f, 158.0f},  /* its left flank */
    {228.0f, 180.0f}, /* the broken pillar */
    {300.0f, 196.0f}, /* the statue */
};

/* What a goal's body asks for: a velocity, and how hard to turn toward it. */
typedef struct {
    float vx, vy;
    float gain;
} steer_t;

/* ---- Small helpers ----------------------------------------------------- */

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

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static float distance(float ax, float ay, float bx, float by)
{
    float dx = ax - bx, dy = ay - by;
    return sqrtf(dx * dx + dy * dy);
}

/* ---- Looking around ---------------------------------------------------- */

static dist_t bucket(float d)
{
    return d < DIST_NEAR_PX ? DIST_NEAR : d < DIST_MID_PX ? DIST_MID : DIST_FAR;
}

/* Each of these returns an index, or -1 if there is no such thing, and
 * reports the distance through *d. */

static int nearest_pellet(const tank_t *t, float x, float y, float *d)
{
    int best = -1;
    for (int i = 0; i < TANK_PELLETS; i++) {
        if (!t->pellets[i].alive) {
            continue;
        }
        float di = distance(x, y, t->pellets[i].x, t->pellets[i].y);
        if (best < 0 || di < *d) {
            best = i;
            *d = di;
        }
    }
    return best;
}

static int nearest_friend(const tank_t *t, int self, float *d)
{
    int best = -1;
    for (int i = 0; i < TANK_FISH; i++) {
        if (i == self) {
            continue;
        }
        float di = distance(t->fish[self].x, t->fish[self].y, t->fish[i].x, t->fish[i].y);
        if (best < 0 || di < *d) {
            best = i;
            *d = di;
        }
    }
    return best;
}

static int nearest_cover(float x, float y, float *d)
{
    int best = -1;
    for (int i = 0; i < TANK_COVERS; i++) {
        float di = distance(x, y, tank_cover[i][0], tank_cover[i][1]);
        if (best < 0 || di < *d) {
            best = i;
            *d = di;
        }
    }
    return best;
}

static void look(const tank_t *t, int i, dist_t seen[SEE_COUNT])
{
    const fish_t *f = &t->fish[i];
    float d = 0.0f;
    seen[SEE_FOOD] = nearest_pellet(t, f->x, f->y, &d) >= 0 ? bucket(d) : DIST_NONE;
    seen[SEE_FRIEND] = nearest_friend(t, i, &d) >= 0 ? bucket(d) : DIST_NONE;
    seen[SEE_FINGER] = t->touching ? bucket(distance(f->x, f->y, t->touch_x, t->touch_y)) : DIST_NONE;
    seen[SEE_COVER] = nearest_cover(f->x, f->y, &d) >= 0 ? bucket(d) : DIST_NONE;
}

static int need_digit(float v)
{
    return clamp_int((int)(v + 0.5f), 0, 9);
}

void tank_percept(const tank_t *t, int fish, percept_t *out)
{
    memset(out, 0, sizeof(*out));
    if (fish < 0 || fish >= TANK_FISH) {
        return;
    }
    const fish_t *f = &t->fish[fish];
    dist_t seen[SEE_COUNT];
    look(t, fish, seen);

    out->species = f->species;
    out->hunger = need_digit(f->hunger);
    out->energy = need_digit(f->energy);
    out->stress = need_digit(f->stress);
    out->bold = f->bold;
    out->social = f->social;
    out->night = t->night;
    out->startled = f->startled;
    out->current = f->goal;
    out->food = seen[SEE_FOOD];
    out->friend_ = seen[SEE_FRIEND];
    out->finger = seen[SEE_FINGER];
    out->cover = seen[SEE_COVER];
}

/* ---- Setting up -------------------------------------------------------- */

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
    f->want_front = rng_range(t, 0.0f, 1.0f) < FRONT_CHANCE; /* and maybe a change of depth */
}

void tank_init_cast(tank_t *t, uint32_t seed, const species_t cast[TANK_FISH])
{
    /* Zero everything, padding included, so two tanks can be compared byte for byte. */
    memset(t, 0, sizeof(*t));
    t->choose = brain_choose;
    t->rng = seed != 0 ? seed : 0x9E3779B9u;
    for (int i = 0; i < 8; i++) {
        rng_next(t); /* stir, so nearby seeds do not start out alike */
    }

    for (int i = 0; i < TANK_FISH; i++) {
        fish_t *f = &t->fish[i];
        species_t kind = CAST[i];
        if (cast != NULL && (int)cast[i] >= 0 && (int)cast[i] < SPECIES_COUNT) {
            kind = cast[i];
        }
        const species_info_t *info = species_info(kind);
        f->species = kind;
        f->bold = clamp_int(info->bold + rng_int(t, -1, 1), 0, 9);
        f->social = clamp_int(info->social + rng_int(t, -1, 1), 0, 9);
        f->x = rng_range(t, SWIM_X_MIN, SWIM_X_MAX);
        f->y = rng_range(t, SWIM_Y_MIN, SWIM_Y_MAX);
        f->facing_left = rng_int(t, 0, 1) == 1;
        f->hunger = rng_range(t, 2.0f, 5.0f);
        float ticks_to_starving = (float)info->hunger_minutes * 60.0f * 1000.0f / TICK_MS;
        f->hunger_per_tick = NEED_MAX / ticks_to_starving *
                             rng_range(t, 1.0f - HUNGER_VARIATION, 1.0f + HUNGER_VARIATION);
        f->energy = rng_range(t, 6.0f, 9.0f);
        f->stress = 0.0f;
        f->goal = GOAL_WANDER;
        f->decide_ticks = 1 + i * DECIDE_STAGGER_TICKS; /* fish take turns */
        pick_wander_target(t, f);
        f->front = f->want_front; /* starts on its chosen layer, at that layer's size */
        f->depth = f->front ? 1.0f : 0.0f;
    }
    for (int i = 0; i < TANK_FISH; i++) {
        look(t, i, t->fish[i].seen); /* so the first tick sees no "change" */
    }
}

void tank_init(tank_t *t, uint32_t seed)
{
    tank_init_cast(t, seed, NULL);
}

void tank_set_brain(tank_t *t, brain_fn fn)
{
    t->choose = fn != NULL ? fn : brain_choose;
}

/* ---- The finger -------------------------------------------------------- */

void tank_set_finger(tank_t *t, bool down, int x, int y)
{
    if (down && !t->finger_down) {
        if (!t->press_pending) { /* remember the first press a tick has not seen */
            t->press_pending = true;
            t->press_x = (float)x;
            t->press_y = (float)y;
        }
    } else if (!down && t->finger_down) {
        t->release_pending = true;
    }
    t->finger_down = down;
    if (down) {
        t->finger_x = (float)x;
        t->finger_y = (float)y;
    }
}

static void drop_pellet(tank_t *t, float x, float y)
{
    for (int i = 0; i < TANK_PELLETS; i++) {
        pellet_t *p = &t->pellets[i];
        if (!p->alive) {
            /* Field by field, not a struct literal: padding must stay zero so
             * two tanks can be compared byte for byte. */
            memset(p, 0, sizeof(*p));
            p->alive = true;
            p->x = clampf(x, SWIM_X_MIN, SWIM_X_MAX);
            p->y = clampf(y, SWIM_Y_MIN, PELLET_FLOOR_Y);
            return;
        }
    }
    /* All slots full: the tap is ignored. */
}

static void begin_touch(tank_t *t, float x, float y)
{
    t->touching = true;
    t->start_x = t->touch_x = x;
    t->start_y = t->touch_y = y;
    t->touch_ticks = 0;
    t->touch_moved = false;

    /* A finger landing close by is a fright, and a bigger one for shy fish. */
    for (int i = 0; i < TANK_FISH; i++) {
        fish_t *f = &t->fish[i];
        if (distance(f->x, f->y, x, y) < STARTLE_PX) {
            int jump = 9 - f->bold;
            f->stress = clampf(f->stress + (float)(jump < 1 ? 1 : jump), 0.0f, NEED_MAX);
            f->startled = true;
        }
    }
}

static void end_touch(tank_t *t)
{
    if (t->touch_ticks <= TAP_MAX_TICKS && !t->touch_moved) {
        drop_pellet(t, t->start_x, t->start_y);
    }
    t->touching = false;
}

static void update_finger(tank_t *t)
{
    bool pressed = t->press_pending;
    bool released = t->release_pending;
    t->press_pending = false;
    t->release_pending = false;

    if (!t->touching) {
        if (pressed) {
            begin_touch(t, t->press_x, t->press_y);
        } else if (t->finger_down) {
            begin_touch(t, t->finger_x, t->finger_y);
        }
    }
    if (!t->touching) {
        return;
    }

    t->touch_ticks++;
    /* A press and release that both happened since the last tick was a touch
     * at the press point; do not let a later re-press drag it elsewhere. */
    bool whole_touch_unseen = pressed && released;
    if (t->finger_down && !whole_touch_unseen) {
        t->touch_x = t->finger_x;
        t->touch_y = t->finger_y;
    }
    if (distance(t->touch_x, t->touch_y, t->start_x, t->start_y) >= TAP_MAX_MOVE_PX) {
        t->touch_moved = true;
    }
    if (released || !t->finger_down) {
        end_touch(t);
    }
}

static void update_pellets(tank_t *t)
{
    for (int i = 0; i < TANK_PELLETS; i++) {
        pellet_t *p = &t->pellets[i];
        if (!p->alive) {
            continue;
        }
        if (!p->settled) {
            p->y += PELLET_SINK_SPEED * TICK_S;
            if (p->y >= PELLET_FLOOR_Y) {
                p->y = PELLET_FLOOR_Y;
                p->settled = true;
                p->settled_ticks = 0;
            }
        } else if (++p->settled_ticks > PELLET_LIFE_TICKS) {
            p->alive = false;
        }
    }
}

/* ---- Needs and decisions ----------------------------------------------- */

static void update_needs(tank_t *t, fish_t *f)
{
    f->hunger += f->hunger_per_tick;
    if (f->goal == GOAL_REST) {
        f->energy += ENERGY_REST_PER_TICK;
    } else {
        f->energy -= ENERGY_DRAIN_PER_TICK * (t->night ? ENERGY_NIGHT_FACTOR : 1.0f);
    }
    f->stress -= STRESS_DECAY_PER_TICK;
    if (t->touching && f->bold <= SHY_BOLD_MAX &&
        distance(f->x, f->y, t->touch_x, t->touch_y) < DIST_NEAR_PX) {
        f->stress += STRESS_DECAY_PER_TICK + STRESS_SHY_PER_TICK; /* outpaces the calming */
    }

    f->hunger = clampf(f->hunger, 0.0f, NEED_MAX);
    f->energy = clampf(f->energy, 0.0f, NEED_MAX);
    f->stress = clampf(f->stress, 0.0f, NEED_MAX);
}

static void maybe_decide(tank_t *t, int i)
{
    fish_t *f = &t->fish[i];
    if (f->sign_ticks > 0) { /* before deciding, so a sign set this tick shows for its full time */
        f->sign_ticks--;
    }
    dist_t now[SEE_COUNT];
    look(t, i, now);
    bool changed = memcmp(now, f->seen, sizeof(now)) != 0;
    memcpy(f->seen, now, sizeof(now));

    /* The regular schedule runs on regardless, so fish that all react to the
     * same event do not end up deciding in step for ever after. */
    f->decide_ticks--;
    bool due = f->decide_ticks <= 0;
    if (due) {
        f->decide_ticks = DECIDE_TICKS;
    }
    if (!due && !f->startled && !changed) {
        return;
    }

    percept_t p;
    tank_percept(t, i, &p);
    choice_t c = t->choose(&p);
    bool usable = (int)c.goal >= 0 && (int)c.goal < GOAL_COUNT && (int)c.mood >= 0 && (int)c.mood < MOOD_COUNT;
    if (usable) { /* anything else is ignored: keep the current goal and mood */
        f->goal = c.goal;
        if (c.mood != f->mood) {
            f->mood = c.mood;
            f->sign_ticks = c.mood == MOOD_CALM ? 0 : SIGN_TICKS;
        }
    }
    f->startled = false;
}

/* ---- Bodies: one per goal ---------------------------------------------- */

/* Head for a point at a speed; stop once within `arrive` of it. Returns true
 * once it has arrived. */
static bool steer_to(const fish_t *f, float tx, float ty, float speed, float arrive, steer_t *s)
{
    float dx = tx - f->x, dy = ty - f->y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d <= arrive || d < 0.001f) {
        s->vx = 0.0f;
        s->vy = 0.0f;
        return true;
    }
    s->vx = dx / d * speed;
    s->vy = dy / d * speed;
    return false;
}

static void body_wander(tank_t *t, fish_t *f, steer_t *s)
{
    if (distance(f->x, f->y, f->tx, f->ty) < WANDER_ARRIVE_PX || f->target_ticks <= 0) {
        pick_wander_target(t, f);
    } else {
        f->target_ticks--;
    }
    steer_to(f, f->tx, f->ty, WANDER_SPEED, 0.0f, s);
}

/* Each of the bodies below returns false if its goal has no target right
 * now, in which case the fish wanders for this tick. */

static bool body_seek_food(tank_t *t, fish_t *f, steer_t *s)
{
    float d = 0.0f;
    int i = nearest_pellet(t, f->x, f->y, &d);
    if (i < 0) {
        return false;
    }
    steer_to(f, t->pellets[i].x, t->pellets[i].y, SEEK_SPEED, 0.0f, s);
    return true;
}

static bool body_rest(tank_t *t, fish_t *f, steer_t *s)
{
    (void)t;
    steer_to(f, f->x, REST_Y, REST_SPEED, REST_ARRIVE_PX, s);
    return true;
}

static bool body_flee(tank_t *t, fish_t *f, steer_t *s)
{
    if (!t->touching) {
        return false;
    }
    float dx = f->x - t->touch_x, dy = f->y - t->touch_y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < 0.001f) { /* right under the finger: any direction will do */
        dx = f->facing_left ? -1.0f : 1.0f;
        dy = 0.0f;
        d = 1.0f;
    }
    s->vx = dx / d * FLEE_SPEED;
    s->vy = dy / d * FLEE_SPEED;
    s->gain = FLEE_GAIN;
    return true;
}

static bool body_investigate(tank_t *t, fish_t *f, steer_t *s)
{
    if (!t->touching) {
        return false;
    }
    float dx = t->touch_x - f->x, dy = t->touch_y - f->y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d > INVESTIGATE_GAP_PX + 8.0f) {
        steer_to(f, t->touch_x, t->touch_y, INVESTIGATE_SPEED, 0.0f, s);
        return true;
    }
    if (d < 0.001f) {
        dx = 1.0f;
        dy = 0.0f;
        d = 1.0f;
    }
    /* Close enough: swim sideways to the finger, which makes a circle, and
     * drift in or out to hold the gap. */
    float in_x = dx / d, in_y = dy / d;
    float drift = (d - INVESTIGATE_GAP_PX) * 0.5f;
    s->vx = -in_y * CIRCLE_SPEED + in_x * drift;
    s->vy = in_x * CIRCLE_SPEED + in_y * drift;
    return true;
}

static bool body_hide(tank_t *t, fish_t *f, steer_t *s)
{
    (void)t;
    float d = 0.0f;
    int i = nearest_cover(f->x, f->y, &d);
    if (i < 0) {
        return false;
    }
    steer_to(f, tank_cover[i][0], tank_cover[i][1], HIDE_SPEED, HIDE_ARRIVE_PX, s);
    return true;
}

static bool body_school(tank_t *t, int self, steer_t *s)
{
    fish_t *f = &t->fish[self];
    float d = 0.0f;
    int i = nearest_friend(t, self, &d);
    if (i < 0) {
        return false;
    }
    const fish_t *friend_ = &t->fish[i];
    if (d > SCHOOL_GAP_PX + SCHOOL_BAND_PX) {
        steer_to(f, friend_->x, friend_->y, SCHOOL_SPEED, 0.0f, s); /* catch up */
        return true;
    }
    /* Close enough: go where the friend is going. A friend that is barely
     * moving is no use as a guide, so wander instead and let it follow us. */
    if (sqrtf(friend_->vx * friend_->vx + friend_->vy * friend_->vy) < SCHOOL_MIN_FRIEND_SPEED) {
        return false;
    }
    s->vx = friend_->vx;
    s->vy = friend_->vy;
    return true;
}

/* A push away from any fish that is too close, on top of what the goal wants. */
static void separate(const tank_t *t, int self, steer_t *s)
{
    const fish_t *f = &t->fish[self];
    for (int i = 0; i < TANK_FISH; i++) {
        if (i == self) {
            continue;
        }
        float dx = f->x - t->fish[i].x, dy = f->y - t->fish[i].y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d >= SEPARATION_PX) {
            continue;
        }
        float push = (1.0f - d / SEPARATION_PX) * SEPARATION_PUSH;
        if (d < 0.001f) { /* exactly on top of each other: split by who came first */
            s->vx += self < i ? -push : push;
            continue;
        }
        s->vx += dx / d * push;
        s->vy += dy / d * push;
    }
}

static void steer(tank_t *t, int i, steer_t *s)
{
    fish_t *f = &t->fish[i];
    bool done = false;
    s->gain = STEER_GAIN;
    switch (f->goal) {
    case GOAL_SEEK_FOOD:
        done = body_seek_food(t, f, s);
        break;
    case GOAL_REST:
        done = body_rest(t, f, s);
        break;
    case GOAL_SCHOOL:
        done = body_school(t, i, s);
        break;
    case GOAL_FLEE:
        done = body_flee(t, f, s);
        break;
    case GOAL_INVESTIGATE:
        done = body_investigate(t, f, s);
        break;
    case GOAL_HIDE:
        done = body_hide(t, f, s);
        break;
    default:
        break;
    }
    if (!done) {
        s->gain = STEER_GAIN;
        body_wander(t, f, s);
    }
    if (f->goal != GOAL_HIDE) { /* a hiding fish holds its spot, even if it has company */
        separate(t, i, s);
    }
}

static void move(fish_t *f, const steer_t *s)
{
    f->vx += (s->vx - f->vx) * s->gain;
    f->vy += (s->vy - f->vy) * s->gain;
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

/* ---- Depth ------------------------------------------------------------- */

/* True if there is no foreground art within `half` pixels of (x, y). */
static bool water_is_clear(float x, float y, float half)
{
    int c0 = (int)floorf((x - half) / ASSET_MASK_CELL), c1 = (int)floorf((x + half) / ASSET_MASK_CELL);
    int r0 = (int)floorf((y - half) / ASSET_MASK_CELL), r1 = (int)floorf((y + half) / ASSET_MASK_CELL);
    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            if (r >= 0 && r < ASSET_MASK_ROWS && c >= 0 && c < ASSET_MASK_COLS && asset_front_mask[r][c]) {
                return false;
            }
        }
    }
    return true;
}

static void update_layer(fish_t *f)
{
    bool hiding = f->goal == GOAL_HIDE;
    bool want = hiding ? false : f->want_front; /* hiding always means behind the art */
    if (f->front == want) {
        f->layer_wait = 0;
    } else if (water_is_clear(f->x, f->y, LAYER_CLEAR_HALF_PX)) {
        f->front = want; /* in open water nobody sees the switch */
        f->layer_wait = 0;
    } else if (hiding && ++f->layer_wait >= LAYER_FORCE_TICKS) {
        f->front = false; /* stuck in front of a rock: better a small pop than hiding in plain view */
        f->layer_wait = 0;
    }

    float target = f->front ? 1.0f : 0.0f;
    f->depth += (target - f->depth) * LAYER_EASE;
    if (fabsf(target - f->depth) < 0.01f) {
        f->depth = target;
    }
}

static void maybe_eat(tank_t *t, fish_t *f)
{
    if (f->goal != GOAL_SEEK_FOOD) {
        return;
    }
    float d = 0.0f;
    int i = nearest_pellet(t, f->x, f->y, &d);
    if (i >= 0 && d < EAT_RADIUS_PX) {
        t->pellets[i].alive = false;
        f->hunger = clampf(f->hunger - HUNGER_PER_PELLET, 0.0f, NEED_MAX);
    }
}

/* ---- One tick ---------------------------------------------------------- */

void tank_tick(tank_t *t, bool night)
{
    t->tick++;
    t->night = night;
    update_finger(t);
    update_pellets(t);

    for (int i = 0; i < TANK_FISH; i++) {
        fish_t *f = &t->fish[i];
        steer_t s;
        update_needs(t, f);
        maybe_decide(t, i);
        steer(t, i, &s);
        move(f, &s);
        update_layer(f);
        maybe_eat(t, f);
    }
}
