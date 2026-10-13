#include <stdlib.h>

#include "app.h"
#include "display.h"
#include "tank.h"
#include "test.h"

extern uint32_t stub_millis;
extern int stub_pushes;

static uint16_t *fb;

static uint32_t ticks_after(uint32_t advance_ms)
{
    uint32_t before = app_tank()->tick;
    stub_millis += advance_ms;
    app_frame(fb);
    return app_tank()->tick - before;
}

static void test_ticks_follow_real_time(void)
{
    stub_millis = 1000;
    app_init(1);
    app_set_speed(1);
    CHECK(app_tank()->tick == 0);
    CHECK(ticks_after(0) == 0);   /* no time passed: draw, but do not advance */
    CHECK(ticks_after(50) == 1);
    CHECK(ticks_after(49) == 0);  /* not a whole tick yet... */
    CHECK(ticks_after(1) == 1);   /* ...and the remainder is kept */
    CHECK(ticks_after(100) == 2);
    CHECK(ticks_after(150) == 3);
}

static void test_every_frame_draws(void)
{
    stub_millis = 0;
    app_init(1);
    stub_pushes = 0;
    ticks_after(0);
    ticks_after(50);
    CHECK(stub_pushes == 2);
}

static void test_long_pause_is_not_replayed(void)
{
    stub_millis = 0;
    app_init(1);
    app_set_speed(1);
    CHECK(ticks_after(10000) == 3); /* ten seconds away: at most three ticks */
    CHECK(ticks_after(50) == 1);    /* and no catching up afterwards */
    CHECK(ticks_after(50) == 1);
}

static void test_clock_wraparound(void)
{
    stub_millis = 0xFFFFFFFFu - 20; /* 20 ms before the 32-bit clock wraps */
    app_init(1);
    app_set_speed(1);
    CHECK(ticks_after(50) == 1);    /* crosses the wrap */
    CHECK(ticks_after(50) == 1);
}

static void test_speed(void)
{
    stub_millis = 0;
    app_init(1);
    app_set_speed(10);
    CHECK(ticks_after(50) == 10);
    CHECK(ticks_after(10000) == 30); /* the cap scales with speed */
    app_set_speed(0);                /* nonsense speeds fall back to 1 */
    CHECK(ticks_after(50) == 1);
    app_set_speed(1);
}

static void test_restart(void)
{
    stub_millis = 0;
    app_init(1);
    ticks_after(50);
    app_init(2);
    CHECK(app_tank()->tick == 0);
}

void test_app(void)
{
    fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    test_ticks_follow_real_time();
    test_every_frame_draws();
    test_long_pause_is_not_replayed();
    test_clock_wraparound();
    test_speed();
    test_restart();
    free(fb);
}
