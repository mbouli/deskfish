#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "app.h"
#include "display.h"
#include "file_helpers.h"
#include "sky.h"
#include "tank.h"
#include "test.h"

extern uint32_t stub_millis;
extern int stub_pushes;
extern int64_t stub_unix;
extern bool stub_touch_down;
extern int stub_touch_x, stub_touch_y;

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

static void test_touch_reaches_the_tank(void)
{
    stub_millis = 0;
    app_init(1);
    app_set_speed(1);
    stub_touch_down = true;
    stub_touch_x = 100;
    stub_touch_y = 100;
    ticks_after(50);
    CHECK(app_tank()->touching);
    stub_touch_down = false;
    ticks_after(50);
    CHECK(!app_tank()->touching);
    int pellets = 0;
    for (int i = 0; i < TANK_PELLETS; i++) {
        pellets += app_tank()->pellets[i].alive;
    }
    CHECK(pellets == 1);
}

/* The sky clock's offset deliberately survives a restart, so each test here
 * begins by putting it back in step with the (stub) real clock. */
static void sync_sky(void)
{
    app_set_sky_rate(1);
    app_skip_sky(stub_unix - app_sky_time());
}

static void test_sky_follows_the_clock(void)
{
    stub_millis = 0;
    stub_unix = sky_unix_from_utc(2026, 6, 21, 18, 0); /* about 1 pm in Auburn */
    CHECK(app_init(1));
    sync_sky();
    ticks_after(50);
    CHECK(app_sky_time() == stub_unix);
    CHECK(app_sky()->sun_up && app_sky()->sun_alt_deg > 60.0f);
    CHECK(!app_tank()->night);

    stub_unix = sky_unix_from_utc(2026, 6, 22, 6, 0);  /* 1 am */
    ticks_after(50);
    CHECK(!app_sky()->sun_up);
    CHECK(app_light()->fish_night);
    CHECK(app_tank()->night);
}

static void test_relight_only_on_a_new_minute(void)
{
    stub_millis = 0;
    stub_unix = sky_unix_from_utc(2026, 6, 21, 18, 0);
    CHECK(app_init(1));
    sync_sky();
    ticks_after(50);
    unsigned first = app_relights();
    CHECK(first >= 1);
    for (int i = 0; i < 20; i++) {
        stub_unix += 2;        /* 40 seconds pass */
        ticks_after(50);
    }
    CHECK(app_relights() == first);
    stub_unix += 25;           /* into the next minute */
    ticks_after(50);
    CHECK(app_relights() == first + 1);
    ticks_after(50);
    CHECK(app_relights() == first + 1);
}

static void test_time_lapse(void)
{
    stub_millis = 0;
    stub_unix = sky_unix_from_utc(2026, 6, 21, 18, 0);
    CHECK(app_init(1));
    sync_sky();
    app_set_sky_rate(3600);    /* an hour of sky per second */
    int64_t start = app_sky_time();
    uint32_t tank_before = app_tank()->tick;
    for (int i = 0; i < 20; i++) { /* one real second, in 50 ms frames */
        ticks_after(50);
    }
    CHECK(app_sky_time() - start >= 3500 && app_sky_time() - start <= 3700);
    CHECK(app_tank()->tick - tank_before == 20); /* the fish did not speed up */

    app_set_sky_rate(1);
    int64_t held = app_sky_time();
    ticks_after(50);
    CHECK(app_sky_time() - held <= 1);           /* and the sky stays where the lapse left it */
    app_set_sky_rate(0);                         /* nonsense rates fall back to 1 */
    ticks_after(50);
    CHECK(app_sky_time() - held <= 1);
}

static void test_sky_jump(void)
{
    stub_millis = 0;
    stub_unix = sky_unix_from_utc(2026, 6, 21, 18, 0);
    CHECK(app_init(1));
    sync_sky();
    ticks_after(50);
    unsigned before = app_relights();
    app_skip_sky(3 * 3600);
    ticks_after(50);
    CHECK(app_sky_time() == stub_unix + 3 * 3600);
    CHECK(app_relights() == before + 1);         /* one re-light, however far it jumped */

    stub_unix += 400 * 86400;                    /* the real clock corrects by over a year */
    ticks_after(50);
    CHECK(app_relights() == before + 2);
    CHECK(isfinite(app_sky()->sun_alt_deg));
    app_skip_sky(-3 * 3600);                     /* and jumping back is allowed */
    ticks_after(50);
    CHECK(app_sky_time() == stub_unix);
}

static void test_app_model(void)
{
    size_t size = 0;
    uint8_t *data = read_file(MODEL_PATH, &size);
    CHECK(data != NULL);
    CHECK(!app_using_model());
    CHECK(!app_set_model(data, 10)); /* cut short: refused, rules stay */
    CHECK(!app_using_model());
    CHECK(app_set_model(data, size));
    free(data);                      /* the app must not need the bytes any more */
    CHECK(app_using_model());
    CHECK(app_init(9));
    CHECK(app_using_model());        /* survives a restart */

    unsigned before = 0, after = 0;
    app_model_stats(&before, NULL, NULL);
    for (int i = 0; i < 200; i++) {
        stub_millis += 50;
        app_frame(fb);
    }
    app_model_stats(&after, NULL, NULL);
    CHECK(after > before);

    app_use_rules(true);
    CHECK(!app_using_model());
    unsigned frozen = 0, later = 0;
    app_model_stats(&frozen, NULL, NULL);
    for (int i = 0; i < 100; i++) {
        stub_millis += 50;
        app_frame(fb);
    }
    app_model_stats(&later, NULL, NULL);
    CHECK(later == frozen);
    app_use_rules(false);
    CHECK(app_using_model());
    app_use_rules(true);             /* leave the app as other tests expect */
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
    test_touch_reaches_the_tank();
    test_sky_follows_the_clock();
    test_relight_only_on_a_new_minute();
    test_time_lapse();
    test_sky_jump();
    test_app_model();
    free(fb);
}
