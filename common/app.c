#include "app.h"

#include "clock.h"
#include "display.h"
#include "render.h"
#include "touch.h"
#include "tuning.h"

#define MAX_TICKS_PER_FRAME 3

static tank_t tank;
static uint32_t last_ms;
static uint32_t owed_ms; /* tank time not yet run */
static int speed = 1;

/* The sky clock is the real clock plus this, in milliseconds so a time-lapse
 * can add a fraction of a second per frame. */
static int64_t sky_offset_ms;
static int sky_rate = 1;
static int64_t lit_minute;
static bool lit;
static sky_t sky;
static light_t light;
static unsigned relights;

static void relight(void)
{
    const place_t place = {PLACE_LAT_DEG, PLACE_LON_DEG};
    sky_compute(app_sky_time(), place, &sky);
    light_compute(&sky, &light);
    relights++;
}

bool app_init(uint32_t seed)
{
    tank_init(&tank, seed);
    last_ms = clock_millis();
    owed_ms = 0;
    lit = false; /* light it on the first frame */
    return true;
}

void app_set_speed(int multiplier)
{
    speed = multiplier < 1 ? 1 : multiplier;
}

void app_set_sky_rate(int multiplier)
{
    sky_rate = multiplier < 1 ? 1 : multiplier;
}

void app_skip_sky(int64_t seconds)
{
    sky_offset_ms += seconds * 1000;
}

int64_t app_sky_time(void)
{
    /* Floor, not truncate, so a negative offset still rounds the same way. */
    int64_t whole = sky_offset_ms >= 0 ? sky_offset_ms / 1000 : -((-sky_offset_ms + 999) / 1000);
    return clock_unix() + whole;
}

const tank_t *app_tank(void)
{
    return &tank;
}

const sky_t *app_sky(void)
{
    return &sky;
}

const light_t *app_light(void)
{
    return &light;
}

unsigned app_relights(void)
{
    return relights;
}

void app_frame(uint16_t *fb)
{
    int tx = 0, ty = 0;
    bool down = touch_read(&tx, &ty);
    tank_set_finger(&tank, down, tx, ty);

    /* Unsigned subtraction gives the right answer even when the clock wraps. */
    uint32_t now = clock_millis();
    uint32_t elapsed = now - last_ms;
    last_ms = now;

    /* After a long pause, do not replay the backlog: cap what we owe. */
    uint32_t cap_ms = (uint32_t)(MAX_TICKS_PER_FRAME * TICK_MS);
    if (elapsed > cap_ms) {
        elapsed = cap_ms;
    }

    /* A time-lapse pushes the sky clock ahead of the real one. */
    sky_offset_ms += (int64_t)elapsed * (sky_rate - 1);

    /* Work the light out again only when the sky clock reaches a new minute. */
    int64_t sky_now = app_sky_time();
    int64_t minute = sky_now >= 0 ? sky_now / 60 : -((-sky_now + 59) / 60);
    if (!lit || minute != lit_minute) {
        relight();
        lit = true;
        lit_minute = minute;
    }

    owed_ms += elapsed * (uint32_t)speed;
    if (owed_ms > cap_ms * (uint32_t)speed) {
        owed_ms = cap_ms * (uint32_t)speed;
    }
    while (owed_ms >= TICK_MS) {
        tank_tick(&tank, light.fish_night);
        owed_ms -= TICK_MS;
    }

    render_tank(&tank, fb);
    display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
}
