#include <stdlib.h>
#include <string.h>

#include "assets.h"
#include "display.h"
#include "light.h"
#include "scene.h"
#include "test.h"

#define ART_TOP (DISPLAY_H - 192)
#define SUN_COLOUR RGB565(255, 246, 214)
#define SUN_CORE RGB565(255, 255, 240)
#define MOON_COLOUR RGB565(228, 233, 248)
#define STAR_COLOUR RGB565(206, 214, 246)
#define STAR_ARM RGB565(150, 160, 214)

static light_t light_at(float alt, bool up, float progress, float phase)
{
    sky_t s = {.sun_alt_deg = alt, .sun_up = up, .progress = progress, .moon_phase = phase};
    light_t l;
    light_compute(&s, &l);
    return l;
}

static uint16_t back(int x, int y)
{
    return scene_backdrop()->px[y * DISPLAY_W + x];
}

static int count_back(uint16_t colour, int y_end)
{
    int n = 0;
    for (int y = 0; y < y_end; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            n += back(x, y) == colour;
        }
    }
    return n;
}

static bool one_of(uint16_t c, const uint16_t *set, int n)
{
    for (int i = 0; i < n; i++) {
        if (c == set[i]) {
            return true;
        }
    }
    return false;
}

static void test_shapes(void)
{
    CHECK(scene_init());
    CHECK(scene_init()); /* twice is fine */
    CHECK(scene_backdrop()->w == DISPLAY_W && scene_backdrop()->h == DISPLAY_H);
    CHECK(scene_front()->w == DISPLAY_W && scene_front()->h == 192);
    CHECK(scene_fish(SPECIES_CATFISH)->w == 16 && scene_fish(SPECIES_CATFISH)->h == 16);
    CHECK(scene_fish(SPECIES_COUNT) == NULL);
    CHECK(scene_fish((species_t)-1) == NULL);
}

static void test_midday_is_the_original_art(void)
{
    light_t l = light_at(60.0f, true, 0.5f, 0.5f);
    scene_relight(&l);

    /* Far from the sun, the sky is the art's water colour and the ruins are
     * the art's own silhouette colour. */
    CHECK(back(4, 4) == asset_water);
    CHECK(back(DISPLAY_W - 4, 60) == asset_water);
    int ruins = 0, ruins_right = 0, sand = 0, sand_right = 0;
    for (int y = ART_TOP; y < DISPLAY_H; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            if (abs(x - l.centre_x) < 150 && y < 150) {
                continue; /* inside the sun's bands */
            }
            uint16_t s = asset_sand.px[(y - ART_TOP) * DISPLAY_W + x];
            if (s != GFX_KEY) {
                sand++;
                sand_right += back(x, y) == s;
            } else if ((asset_far_mask[y - ART_TOP][x >> 3] >> (x & 7)) & 1) {
                ruins++;
                ruins_right += back(x, y) == RGB565(52, 109, 214);
            }
        }
    }
    CHECK(ruins > 100 && ruins_right == ruins);
    CHECK(sand > 5000 && sand_right == sand);

    /* With no tint, the foreground, fish and pellet are untouched. */
    CHECK(memcmp(scene_front()->px, asset_front.px, DISPLAY_W * 192 * sizeof(uint16_t)) == 0);
    for (int s = 0; s < SPECIES_COUNT; s++) {
        CHECK(memcmp(scene_fish((species_t)s)->px, asset_fish[s].px, 256 * sizeof(uint16_t)) == 0);
    }
    CHECK(scene_pellet() == RGB565(214, 170, 96));

    /* The sun is there, with its paler centre. */
    CHECK(back(l.centre_x, l.centre_y) == SUN_CORE);
    CHECK(count_back(SUN_COLOUR, ART_TOP) > 50);
    CHECK(count_back(MOON_COLOUR, DISPLAY_H) == 0);
    CHECK(count_back(STAR_COLOUR, DISPLAY_H) == 0);
}

static void test_sky_is_flat_bands_only(void)
{
    /* In the open sky above the art, every pixel is one of a handful of colours. */
    light_t l = light_at(1.0f, true, 0.97f, 0.5f); /* sunset */
    scene_relight(&l);
    uint16_t allowed[LIGHT_BANDS + 2];
    memcpy(allowed, l.band_colour, sizeof(l.band_colour));
    allowed[LIGHT_BANDS] = SUN_COLOUR;
    allowed[LIGHT_BANDS + 1] = SUN_CORE;
    bool ok = true;
    bool seen[LIGHT_BANDS] = {false};
    for (int y = 0; y < ART_TOP; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            ok = ok && one_of(back(x, y), allowed, LIGHT_BANDS + 2);
            for (int i = 0; i < LIGHT_BANDS; i++) {
                seen[i] = seen[i] || back(x, y) == l.band_colour[i];
            }
        }
    }
    CHECK(ok);
    int bands_seen = 0;
    for (int i = 0; i < LIGHT_BANDS; i++) {
        bands_seen += seen[i];
    }
    CHECK(bands_seen >= 4); /* a real spread of colour across the top of the screen */

    /* Warm on the sun's side, dark on the other. */
    uint16_t sunny = back(DISPLAY_W - 30, 40), far_side = back(8, 8);
    CHECK((sunny >> 11) > (sunny & 0x1F));
    CHECK(far_side == l.band_colour[LIGHT_BANDS - 1] || far_side == l.band_colour[LIGHT_BANDS - 2]);
}

static void test_edges_are_streaky_and_stepped(void)
{
    light_t l = light_at(1.0f, true, 0.5f, 0.5f); /* horizon colours, sun mid-screen */
    scene_relight(&l);
    /* Along rows above the art, find where band 3 gives way to band 4 on the
     * right of the sun, and count how often that edge moves from one row to
     * the next. */
    int last = -1, differing = 0;
    for (int y = 4; y < ART_TOP - 2; y++) {
        int edge = -1;
        for (int x = l.centre_x; x < DISPLAY_W - 1; x++) {
            if (back(x, y) == l.band_colour[3] && back(x + 1, y) == l.band_colour[4]) {
                edge = x;
                break;
            }
        }
        if (edge >= 0 && last >= 0 && edge != last) {
            differing++;
        }
        if (edge >= 0) {
            last = edge;
        }
    }
    CHECK(differing > 8); /* it wanders from row to row */
}

static void test_night_sky(void)
{
    light_t l = light_at(-40.0f, false, 0.5f, 0.5f); /* midnight, full moon */
    scene_relight(&l);
    uint16_t allowed[] = {l.band_colour[0], l.halo_colour[0], l.halo_colour[1], l.halo_colour[2],
                          MOON_COLOUR, STAR_COLOUR, STAR_ARM};
    bool ok = true;
    for (int y = 0; y < ART_TOP; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            ok = ok && one_of(back(x, y), allowed, 7);
        }
    }
    CHECK(ok);
    CHECK(count_back(STAR_COLOUR, DISPLAY_H) >= 12); /* many are hidden behind sand, or dimmed by the ruins */
    CHECK(count_back(SUN_COLOUR, DISPLAY_H) == 0 && count_back(SUN_CORE, DISPLAY_H) == 0);
    CHECK(count_back(l.halo_colour[0], ART_TOP) > 200);

    /* The scene is dimmed: the foreground is darker than the art, and still keyed. */
    bool darker = true, keys_kept = true;
    for (int i = 0; i < DISPLAY_W * 192; i++) {
        uint16_t src = asset_front.px[i], lit = scene_front()->px[i];
        keys_kept = keys_kept && (src == GFX_KEY) == (lit == GFX_KEY);
        if (src != GFX_KEY) {
            darker = darker && (lit >> 11) <= (src >> 11) && (lit & 0x1F) <= (src & 0x1F);
        }
    }
    CHECK(darker && keys_kept);
    CHECK(scene_pellet() != RGB565(214, 170, 96));
}

static int moon_pixels(float phase, int *left, int *right)
{
    light_t l = light_at(-40.0f, false, 0.5f, phase); /* the moon high and clear of the art */
    scene_relight(&l);
    int n = 0;
    *left = *right = 0;
    for (int y = 0; y < ART_TOP; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            if (back(x, y) == MOON_COLOUR) {
                n++;
                *left += x < l.moon_x;
                *right += x > l.moon_x;
            }
        }
    }
    return n;
}

static void test_moon_phases(void)
{
    int left, right;
    int full = moon_pixels(0.5f, &left, &right);
    CHECK(full > 250 && full < 340);                 /* a disc of radius 10 */
    CHECK(moon_pixels(0.0f, &left, &right) == 0);    /* a new moon is invisible */

    int first_quarter = moon_pixels(0.25f, &left, &right);
    CHECK(first_quarter > full * 2 / 5 && first_quarter < full * 3 / 5);
    CHECK(right > left * 5);                         /* waxing: lit on the right */

    int last_quarter = moon_pixels(0.75f, &left, &right);
    CHECK(last_quarter > full * 2 / 5 && last_quarter < full * 3 / 5);
    CHECK(left > right * 5);                         /* waning: lit on the left */

    int crescent = moon_pixels(0.1f, &left, &right);
    CHECK(crescent > 0 && crescent < first_quarter);
    CHECK(moon_pixels(0.4f, &left, &right) > first_quarter);
}

static void test_relight_is_repeatable(void)
{
    light_t l = light_at(3.0f, true, 0.02f, 0.3f);
    scene_relight(&l);
    unsigned count = scene_relight_count();
    uint16_t *copy = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    memcpy(copy, scene_backdrop()->px, DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    light_t other = light_at(-40.0f, false, 0.5f, 0.5f);
    scene_relight(&other);
    scene_relight(&l);
    CHECK(memcmp(copy, scene_backdrop()->px, DISPLAY_W * DISPLAY_H * sizeof(uint16_t)) == 0);
    CHECK(scene_relight_count() == count + 2);
    free(copy);
}

static void test_tint_never_makes_a_hole(void)
{
    /* Whatever the light, an opaque pixel must stay opaque. */
    bool ok = true;
    for (float alt = -40.0f; alt <= 60.0f; alt += 2.5f) {
        for (int p = 0; p <= 2; p++) {
            light_t l = light_at(alt, alt > -0.833f, 0.5f, (float)p / 2.0f);
            scene_relight(&l);
            for (int i = 0; i < DISPLAY_W * 192; i++) {
                ok = ok && (asset_front.px[i] == GFX_KEY) == (scene_front()->px[i] == GFX_KEY);
            }
            for (int s = 0; s < SPECIES_COUNT; s++) {
                for (int i = 0; i < 256; i++) {
                    ok = ok && (asset_fish[s].px[i] == GFX_KEY) == (scene_fish((species_t)s)->px[i] == GFX_KEY);
                }
            }
            for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
                ok = ok && scene_backdrop()->px[i] != GFX_KEY;
            }
        }
    }
    CHECK(ok);
}

static void test_sun_at_the_screen_edge(void)
{
    /* The sun and moon enter and leave at the edges: discs and bands must clip. */
    for (float progress = 0.0f; progress <= 1.0f; progress += 0.05f) {
        light_t day = light_at(0.5f, true, progress, 0.5f);
        scene_relight(&day);
        light_t night = light_at(-30.0f, false, progress, 0.5f);
        scene_relight(&night);
    }
    light_t odd = light_at(10.0f, true, 0.5f, 0.5f);
    odd.centre_x = -40;   /* nonsense positions must not write outside the picture */
    odd.centre_y = 400;
    odd.moon_x = 5000;
    odd.moon_y = -5000;
    odd.show_moon = true;
    odd.show_halo = true;
    odd.star_count = 9999;
    scene_relight(&odd);
    CHECK(scene_relight_count() > 0); /* reaching here under AddressSanitizer is the test */
}

void test_scene(void)
{
    test_shapes();
    test_midday_is_the_original_art();
    test_sky_is_flat_bands_only();
    test_edges_are_streaky_and_stepped();
    test_night_sky();
    test_moon_phases();
    test_relight_is_repeatable();
    test_tint_never_makes_a_hole();
    test_sun_at_the_screen_edge();
}
