#include <math.h>
#include <stdlib.h>

#include "assets.h"
#include "display.h"
#include "light.h"
#include "test.h"

static const place_t AUBURN = {32.61f, -85.48f};

static light_t at(float alt, bool up, float progress, float phase)
{
    sky_t s = {.sun_alt_deg = alt, .sun_up = up, .progress = progress, .moon_phase = phase};
    light_t l;
    light_compute(&s, &l);
    return l;
}

/* Distance between two RGB565 colours, as the largest step in any one channel. */
static int colour_step(uint16_t a, uint16_t b)
{
    int dr = abs((a >> 11) - (b >> 11));
    int dg = abs(((a >> 5) & 0x3F) - ((b >> 5) & 0x3F)) / 2; /* green has twice the steps */
    int db = abs((a & 0x1F) - (b & 0x1F));
    return dr > dg ? (dr > db ? dr : db) : (dg > db ? dg : db);
}

static void test_bands_are_ordered(void)
{
    bool ok = true;
    for (float alt = -30.0f; alt <= 85.0f; alt += 0.25f) {
        light_t l = at(alt, alt > -0.833f, 0.3f, 0.5f);
        ok = ok && l.band_radius[0] > 0;
        for (int i = 1; i < LIGHT_BANDS - 1; i++) {
            ok = ok && l.band_radius[i] > l.band_radius[i - 1];
        }
    }
    CHECK(ok);
}

static void test_high_sun_matches_the_art(void)
{
    light_t l = at(60.0f, true, 0.5f, 0.5f);
    CHECK(l.band_colour[LIGHT_BANDS - 1] == RGB565(62, 121, 221));
    CHECK(l.band_colour[LIGHT_BANDS - 1] == asset_water);
    CHECK(l.tint[0] == 1.0f && l.tint[1] == 1.0f && l.tint[2] == 1.0f);
    CHECK(l.show_sun && !l.show_moon && !l.show_halo);
    CHECK(l.star_count == 0);
    CHECK(!l.fish_night);
    CHECK(l.band_colour[0] != l.band_colour[LIGHT_BANDS - 1]); /* there are bands */
}

static void test_deep_night(void)
{
    light_t l = at(-40.0f, false, 0.5f, 0.5f);
    for (int i = 0; i < LIGHT_BANDS; i++) {
        CHECK(l.band_colour[i] == RGB565(14, 18, 52)); /* one flat dark sky */
    }
    CHECK(!l.show_sun && l.show_moon && l.show_halo);
    CHECK(l.halo_colour[0] == RGB565(46, 54, 110));
    CHECK(l.halo_colour[1] == RGB565(30, 36, 84));
    CHECK(l.halo_colour[2] == RGB565(22, 26, 68));
    CHECK(l.star_count == LIGHT_MAX_STARS);
    CHECK(l.fish_night);
    CHECK(l.tint[0] < 0.5f && l.tint[2] > l.tint[0]); /* dark, and bluer than it is red */
    CHECK(l.moon_phase == 0.5f);
}

static void test_sunset_is_warm_near_the_sun_and_dark_far_away(void)
{
    light_t l = at(1.0f, true, 0.99f, 0.5f);
    uint16_t near = l.band_colour[0], far = l.band_colour[LIGHT_BANDS - 1];
    CHECK((near >> 11) > (near & 0x1F));       /* near the sun: more red than blue */
    CHECK((far & 0x1F) > (far >> 11));         /* opposite: more blue than red */
    CHECK((far >> 11) < 8);                    /* and dark */
    CHECK(l.centre_x > DISPLAY_W * 3 / 4);     /* the sun is on the right at sunset */
    light_t morning = at(1.0f, true, 0.01f, 0.5f);
    CHECK(morning.centre_x < DISPLAY_W / 4);   /* and on the left at sunrise */
    CHECK(morning.band_colour[0] == l.band_colour[0]); /* same colours, mirrored */
}

static void test_sun_path(void)
{
    int last_x = -1;
    bool ok = true;
    for (int i = 0; i <= 100; i++) {
        light_t l = at(30.0f, true, (float)i / 100.0f, 0.5f);
        ok = ok && l.centre_x > last_x - 1 && l.centre_x >= 0 && l.centre_x < DISPLAY_W;
        ok = ok && l.centre_y >= 0 && l.centre_y < DISPLAY_H;
        last_x = l.centre_x;
    }
    CHECK(ok);
    CHECK(at(30.0f, true, 0.5f, 0.5f).centre_y < at(30.0f, true, 0.0f, 0.5f).centre_y - 60); /* high at noon */
    CHECK(at(30.0f, true, 0.0f, 0.5f).centre_y == at(30.0f, true, 1.0f, 0.5f).centre_y);

    /* Just after sunset the bands stay where the sun went down, and sink. */
    light_t set = at(-0.9f, false, 0.0f, 0.5f);
    light_t day_end = at(-0.8f, true, 1.0f, 0.5f);
    CHECK(set.centre_x == day_end.centre_x);
    CHECK(abs(set.centre_y - day_end.centre_y) <= 6);
    CHECK(at(-8.0f, false, 0.05f, 0.5f).centre_y > set.centre_y);
    /* Before sunrise they are on the other side. */
    CHECK(at(-3.0f, false, 0.98f, 0.5f).centre_x < DISPLAY_W / 4);

    /* The moon crosses the night the way the sun crosses the day. */
    CHECK(at(-30.0f, false, 0.1f, 0.5f).moon_x < at(-30.0f, false, 0.9f, 0.5f).moon_x);
    CHECK(at(-30.0f, false, 0.5f, 0.5f).moon_y < at(-30.0f, false, 0.0f, 0.5f).moon_y - 60);
}

static void test_stars_and_night_come_in_gradually(void)
{
    int last = LIGHT_MAX_STARS;
    bool ok = true;
    for (float alt = -20.0f; alt <= 10.0f; alt += 0.25f) {
        light_t l = at(alt, alt > -0.833f, 0.5f, 0.5f);
        ok = ok && l.star_count <= last && l.star_count >= 0 && l.star_count <= LIGHT_MAX_STARS;
        ok = ok && (alt < -4.0f || l.star_count == 0);
        ok = ok && l.show_sun == (alt > -0.833f) && l.show_moon == !l.show_sun;
        last = l.star_count;
    }
    CHECK(ok);
    CHECK(!at(-5.9f, false, 0.1f, 0.5f).fish_night);
    CHECK(at(-6.1f, false, 0.1f, 0.5f).fish_night);
    CHECK(!at(-5.9f, false, 0.1f, 0.5f).show_halo);
    CHECK(at(-9.0f, false, 0.1f, 0.5f).show_halo);
}

static void test_full_moon_is_brighter(void)
{
    light_t full = at(-40.0f, false, 0.5f, 0.5f), new_ = at(-40.0f, false, 0.5f, 0.0f);
    CHECK(full.tint[0] > new_.tint[0] && full.tint[1] > new_.tint[1] && full.tint[2] > new_.tint[2]);
    CHECK(full.tint[0] < 0.5f);
    /* The moon makes no difference by day. */
    CHECK(at(40.0f, true, 0.5f, 0.5f).tint[0] == at(40.0f, true, 0.5f, 0.0f).tint[0]);
}

static void test_a_real_day_has_no_jumps(void)
{
    /* Walk a whole midsummer day and night a minute at a time. */
    int64_t start = sky_unix_from_utc(2026, 6, 21, 0, 0);
    sky_t s;
    light_t last, l;
    sky_compute(start, AUBURN, &s);
    light_compute(&s, &last);
    int worst_colour = 0, worst_radius = 0, worst_centre = 0;
    for (int64_t t = start + 60; t < start + 86400 + 3600; t += 60) {
        sky_compute(t, AUBURN, &s);
        light_compute(&s, &l);
        for (int i = 0; i < LIGHT_BANDS; i++) {
            int step = colour_step(l.band_colour[i], last.band_colour[i]);
            worst_colour = step > worst_colour ? step : worst_colour;
        }
        for (int i = 0; i < LIGHT_BANDS - 1; i++) {
            int step = abs(l.band_radius[i] - last.band_radius[i]);
            worst_radius = step > worst_radius ? step : worst_radius;
        }
        if (s.sun_alt_deg > -12.0f) { /* in full night the bands are one colour and their centre is invisible */
            int step = abs(l.centre_x - last.centre_x) + abs(l.centre_y - last.centre_y);
            worst_centre = step > worst_centre ? step : worst_centre;
        }
        last = l;
    }
    CHECK(worst_colour <= 3);  /* out of 31 */
    CHECK(worst_radius <= 6);  /* pixels */
    CHECK(worst_centre <= 8);  /* pixels: while they can be seen, the bands never leap across the sky */
}

void test_light(void)
{
    test_bands_are_ordered();
    test_high_sun_matches_the_art();
    test_deep_night();
    test_sunset_is_warm_near_the_sun_and_dark_far_away();
    test_sun_path();
    test_stars_and_night_come_in_gradually();
    test_full_moon_is_brighter();
    test_a_real_day_has_no_jumps();
}
