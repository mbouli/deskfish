#pragma once

/* Every behaviour number lives here, so tuning by eye means editing one file.
 * Distances are screen pixels, speeds are pixels per second, and "per tick"
 * means per 1/20 second. */

/* --- Where fish can be -------------------------------------------------- */

/* Where a fish's centre may be. Fish are 32 pixels wide, so this keeps the
 * whole sprite on screen sideways. */
#define SWIM_X_MIN 16.0f
#define SWIM_X_MAX 304.0f
#define SWIM_Y_MIN 24.0f
#define SWIM_Y_MAX 200.0f

/* Preferred depth bands (centre y). */
#define DEPTH_UPPER_MIN 36.0f
#define DEPTH_UPPER_MAX 90.0f
#define DEPTH_MIDDLE_MIN 80.0f
#define DEPTH_MIDDLE_MAX 150.0f
#define DEPTH_BOTTOM_MIN 150.0f
#define DEPTH_BOTTOM_MAX 200.0f

/* --- Time of day -------------------------------------------------------- */

#define DAY_START_MIN (6 * 60)
#define NIGHT_START_MIN (20 * 60)

/* --- Needs, 0..9 -------------------------------------------------------- */

#define NEED_MAX 9.0f
#define HUNGER_VARIATION 0.2f         /* a fish's appetite is its species' rate, give or take this fraction */
#define HUNGER_PER_PELLET 4.0f
#define ENERGY_DRAIN_PER_TICK 0.0015f /* full to empty in 5 minutes of activity */
#define ENERGY_NIGHT_FACTOR 3.0f      /* activity costs this much more at night */
#define ENERGY_REST_PER_TICK 0.01f    /* empty to full in 45 seconds of rest */
#define STRESS_DECAY_PER_TICK 0.01f   /* 9 to 0 in 45 seconds */
#define STRESS_SHY_PER_TICK 0.05f     /* a shy fish with a finger close by */
#define SHY_BOLD_MAX 3                /* "shy" means bold at or below this */

/* --- Seeing and deciding ------------------------------------------------ */

#define DIST_NEAR_PX 48.0f
#define DIST_MID_PX 120.0f
#define DECIDE_TICKS 30               /* ask the brain every 1.5 seconds */
#define DECIDE_STAGGER_TICKS 6        /* fish take turns */

/* --- The finger and food ------------------------------------------------ */

#define TAP_MAX_TICKS 6               /* longer than 0.3 seconds is a hold */
#define TAP_MAX_MOVE_PX 10.0f
#define STARTLE_PX 40.0f
#define PELLET_SINK_SPEED 15.0f
#define PELLET_FLOOR_Y 206.0f         /* where pellets come to rest */
#define PELLET_LIFE_TICKS 600         /* 30 seconds on the sand */
#define EAT_RADIUS_PX 10.0f

/* --- Movement ----------------------------------------------------------- */

#define STEER_GAIN 0.08f              /* fraction of the way to the wanted velocity, per tick */
#define FACING_FLIP_SPEED 3.0f        /* sideways speed needed before the sprite turns round */

#define WANDER_SPEED 20.0f
#define WANDER_ARRIVE_PX 8.0f         /* close enough: pick a new point */
#define WANDER_MIN_TICKS 80           /* give up on a point after 4 to 12 seconds */
#define WANDER_MAX_TICKS 240

#define SEEK_SPEED 45.0f

#define REST_SPEED 6.0f
#define REST_Y 188.0f
#define REST_ARRIVE_PX 4.0f

#define FLEE_SPEED 90.0f
#define FLEE_GAIN 0.3f                /* fleeing fish turn hard */

#define INVESTIGATE_SPEED 30.0f
#define INVESTIGATE_GAP_PX 30.0f      /* how close a curious fish comes */
#define CIRCLE_SPEED 15.0f

#define HIDE_SPEED 50.0f
#define HIDE_ARRIVE_PX 3.0f

#define SCHOOL_SPEED 28.0f
#define SCHOOL_GAP_PX 30.0f           /* the distance schooling fish keep */
#define SCHOOL_BAND_PX 10.0f
#define SCHOOL_MIN_FRIEND_SPEED 5.0f  /* a friend slower than this is not worth following */

#define SEPARATION_PX 22.0f           /* closer than this, fish push apart */
#define SEPARATION_PUSH 45.0f
