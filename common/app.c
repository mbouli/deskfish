#include "app.h"

#include "clock.h"
#include "display.h"
#include "render.h"
#include "touch.h"

#define MAX_TICKS_PER_FRAME 3

static tank_t tank;
static uint32_t last_ms;
static uint32_t owed_ms; /* tank time not yet run */
static int speed = 1;
static app_time_t time_mode = APP_TIME_REAL;

void app_init(uint32_t seed)
{
    tank_init(&tank, seed);
    last_ms = clock_millis();
    owed_ms = 0;
}

void app_set_speed(int multiplier)
{
    speed = multiplier < 1 ? 1 : multiplier;
}

void app_set_time(app_time_t mode)
{
    time_mode = mode;
}

const tank_t *app_tank(void)
{
    return &tank;
}

static int minutes_now(void)
{
    switch (time_mode) {
    case APP_TIME_NIGHT:
        return 0;
    case APP_TIME_DAY:
        return 12 * 60;
    case APP_TIME_REAL:
    default:
        return clock_minutes();
    }
}

void app_frame(uint16_t *fb)
{
    int minutes = minutes_now();

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
    owed_ms += elapsed * (uint32_t)speed;
    if (owed_ms > cap_ms * (uint32_t)speed) {
        owed_ms = cap_ms * (uint32_t)speed;
    }

    while (owed_ms >= TICK_MS) {
        tank_tick(&tank, minutes);
        owed_ms -= TICK_MS;
    }

    render_tank(&tank, minutes, fb);
    display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
}
