#include <math.h>

#include "sky.h"
#include "test.h"

static const place_t AUBURN = {32.61f, -85.48f};

/* The first minute at or after `from` when sun_up differs from the minute before. */
static int64_t next_flip(int64_t from, place_t place)
{
    sky_t s;
    sky_compute(from, place, &s);
    bool up = s.sun_up;
    for (int64_t t = from + 60; t < from + 2 * 86400; t += 60) {
        sky_compute(t, place, &s);
        if (s.sun_up != up) {
            return t;
        }
    }
    return -1;
}

static void check_day(int y, int m, int d, double rise_min, double set_min)
{
    int64_t midnight = sky_unix_from_utc(y, m, d, 0, 0);
    int64_t rise = next_flip(midnight + 6 * 3600, AUBURN); /* 06:00 UTC is the small hours in Auburn */
    int64_t set = next_flip(rise, AUBURN);
    CHECK(fabs((double)(rise - midnight) / 60.0 - rise_min) <= 3.0);
    CHECK(fabs((double)(set - midnight) / 60.0 - set_min) <= 3.0);

    sky_t s;
    sky_compute(rise, AUBURN, &s);
    CHECK(s.sun_up && s.progress < 0.01f);          /* the day has just begun */
    sky_compute(set - 60, AUBURN, &s);
    CHECK(s.sun_up && s.progress > 0.99f);          /* and is nearly over */
    sky_compute(set, AUBURN, &s);
    CHECK(!s.sun_up && s.progress < 0.01f);         /* the night has just begun */
}

static void test_sunrise_and_sunset(void)
{
    check_day(2026, 3, 20, 705.2, 1433.4);
    check_day(2026, 6, 21, 634.9, 1492.7);
    check_day(2026, 9, 22, 690.4, 1418.8);
    check_day(2026, 12, 21, 760.1, 1360.2);
}

static void test_noon_height(void)
{
    /* At local solar noon the sun stands at 90 - latitude + declination. */
    sky_t s;
    int64_t midnight = sky_unix_from_utc(2026, 6, 21, 0, 0);
    float best = -90.0f;
    int64_t best_t = 0;
    for (int64_t t = midnight; t < midnight + 86400 + 6 * 3600; t += 60) {
        sky_compute(t, AUBURN, &s);
        if (s.sun_alt_deg > best) {
            best = s.sun_alt_deg;
            best_t = t;
        }
    }
    CHECK(fabsf(best - 80.83f) < 0.3f);                               /* midsummer */
    CHECK(fabs((double)(best_t - midnight) / 60.0 - 1063.8) <= 3.0);  /* solar noon */
    sky_compute(best_t, AUBURN, &s);
    CHECK(fabsf(s.progress - 0.5f) < 0.01f);

    sky_compute(sky_unix_from_utc(2026, 12, 21, 17, 40), AUBURN, &s); /* midwinter noon */
    CHECK(fabsf(s.sun_alt_deg - 33.95f) < 0.3f);

    sky_compute(sky_unix_from_utc(2026, 6, 21, 6, 0), AUBURN, &s);    /* 1 am local */
    CHECK(s.sun_alt_deg < -20.0f && !s.sun_up);
}

static void test_progress_is_smooth(void)
{
    sky_t s;
    int64_t start = sky_unix_from_utc(2026, 10, 12, 0, 0);
    sky_compute(start, AUBURN, &s);
    bool up = s.sun_up, ok = true;
    float last = s.progress;
    for (int64_t t = start + 60; t < start + 3 * 86400; t += 60) {
        sky_compute(t, AUBURN, &s);
        ok = ok && s.progress >= 0.0f && s.progress <= 1.0f;
        if (s.sun_up == up) {
            ok = ok && s.progress >= last && s.progress - last < 0.01f; /* creeps forward */
        } else {
            ok = ok && last > 0.99f && s.progress < 0.01f;              /* starts again from 0 */
            up = s.sun_up;
        }
        last = s.progress;
    }
    CHECK(ok);
}

static void test_polar_day_and_night(void)
{
    static const place_t places[] = {{90.0f, 0.0f}, {-90.0f, 0.0f}, {89.9f, 120.0f}, {0.0f, 0.0f},
                                     {69.65f, 18.96f} /* Tromso */, {-77.85f, 166.67f} /* McMurdo */};
    bool ok = true;
    for (unsigned p = 0; p < sizeof(places) / sizeof(places[0]); p++) {
        for (int month = 1; month <= 12; month++) {
            int64_t start = sky_unix_from_utc(2026, month, 21, 0, 0);
            for (int64_t t = start; t < start + 86400; t += 600) {
                sky_t s;
                sky_compute(t, places[p], &s);
                ok = ok && isfinite(s.sun_alt_deg) && s.sun_alt_deg >= -90.0f && s.sun_alt_deg <= 90.0f;
                ok = ok && isfinite(s.progress) && s.progress >= 0.0f && s.progress <= 1.0f;
            }
        }
    }
    CHECK(ok);

    /* Tromso: no sunrise at midwinter, no sunset at midsummer. */
    const place_t tromso = {69.65f, 18.96f};
    bool ever_up = false, ever_down = false;
    for (int minute = 0; minute < 1440; minute += 10) {
        sky_t s;
        sky_compute(sky_unix_from_utc(2026, 12, 21, 0, 0) + minute * 60, tromso, &s);
        ever_up = ever_up || s.sun_up;
        sky_compute(sky_unix_from_utc(2026, 6, 21, 0, 0) + minute * 60, tromso, &s);
        ever_down = ever_down || !s.sun_up;
    }
    CHECK(!ever_up);
    CHECK(!ever_down);
}

/* Distance from a phase to a target, the short way round the circle. */
static float phase_gap(float phase, float target)
{
    float d = fabsf(phase - target);
    return d > 0.5f ? 1.0f - d : d;
}

static void test_moon_phase(void)
{
    sky_t s;
    sky_compute(sky_unix_from_utc(2017, 8, 21, 18, 0), AUBURN, &s); /* solar eclipse: new */
    CHECK(phase_gap(s.moon_phase, 0.0f) < 0.034f);                  /* 0.034 of a cycle is one day */
    sky_compute(sky_unix_from_utc(2024, 4, 8, 18, 0), AUBURN, &s);  /* solar eclipse: new */
    CHECK(phase_gap(s.moon_phase, 0.0f) < 0.034f);
    sky_compute(sky_unix_from_utc(2025, 3, 14, 7, 0), AUBURN, &s);  /* lunar eclipse: full */
    CHECK(phase_gap(s.moon_phase, 0.5f) < 0.034f);

    /* A week after new it is about first quarter, and it only ever moves forward. */
    sky_compute(sky_unix_from_utc(2024, 4, 15, 18, 0), AUBURN, &s);
    CHECK(phase_gap(s.moon_phase, 0.25f) < 0.04f);
    bool ok = true;
    float last = -1.0f;
    for (int day = 0; day < 20; day++) {
        sky_compute(sky_unix_from_utc(2024, 4, 9, 18, 0) + day * 86400, AUBURN, &s);
        ok = ok && s.moon_phase > last && s.moon_phase >= 0.0f && s.moon_phase < 1.0f;
        last = s.moon_phase;
    }
    CHECK(ok);
}

static void test_negative_time(void)
{
    sky_t s;
    sky_compute(sky_unix_from_utc(1960, 7, 4, 12, 0), AUBURN, &s);
    CHECK(sky_unix_from_utc(1960, 7, 4, 12, 0) < 0);
    CHECK(isfinite(s.sun_alt_deg) && s.progress >= 0.0f && s.progress <= 1.0f);
    CHECK(s.moon_phase >= 0.0f && s.moon_phase < 1.0f);
}

static void test_unix_from_utc(void)
{
    CHECK(sky_unix_from_utc(1970, 1, 1, 0, 0) == 0);
    CHECK(sky_unix_from_utc(1970, 1, 2, 0, 1) == 86460);
    CHECK(sky_unix_from_utc(2000, 1, 1, 0, 0) == 946684800);
    CHECK(sky_unix_from_utc(2000, 1, 6, 18, 14) == 947182440);
    CHECK(sky_unix_from_utc(2024, 2, 29, 0, 0) == 1709164800); /* a leap day */
    CHECK(sky_unix_from_utc(2024, 3, 1, 0, 0) == 1709251200);
    CHECK(sky_unix_from_utc(1969, 12, 31, 23, 59) == -60);
}

void test_sky(void)
{
    test_unix_from_utc();
    test_sunrise_and_sunset();
    test_noon_height();
    test_progress_is_smooth();
    test_polar_day_and_night();
    test_moon_phase();
    test_negative_time();
}
