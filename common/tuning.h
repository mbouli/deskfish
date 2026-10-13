#pragma once

/* Every behaviour number lives here, so tuning by eye means editing one file.
 * Distances are screen pixels, speeds are pixels per second. */

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

#define WANDER_SPEED 20.0f
#define WANDER_ARRIVE_PX 8.0f   /* close enough: pick a new point */
#define WANDER_MIN_TICKS 80     /* give up on a point after 4 to 12 seconds */
#define WANDER_MAX_TICKS 240

#define STEER_GAIN 0.08f        /* fraction of the way to the wanted velocity, per tick */
#define FACING_FLIP_SPEED 3.0f  /* sideways speed needed before the sprite turns round */
