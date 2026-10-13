#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "light.h"
#include "render.h"
#include "scene.h"
#include "tank_helpers.h"
#include "test.h"

/* Written out here, not borrowed from the code under test. */
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define OUTLINE C565(20, 28, 60)
#define PIXELS (DISPLAY_W * DISPLAY_H)

/* The colours each mood's sign may use, ending in 0. Black is never one of them. */
static const uint16_t allowed[MOOD_COUNT][6] = {
    [MOOD_CALM] = {0},
    [MOOD_EAGER] = {C565(255, 214, 64), C565(255, 150, 40), 0},
    [MOOD_WARY] = {C565(255, 128, 48), OUTLINE, 0},
    [MOOD_PLAYFUL] = {C565(255, 128, 190), OUTLINE, 0},
    [MOOD_SLEEPY] = {C565(190, 220, 255), OUTLINE, 0},
    [MOOD_SULKY] = {C565(196, 188, 216), C565(150, 140, 180), C565(108, 98, 140), C565(96, 176, 255), OUTLINE, 0},
};

static uint16_t *blank(void)
{
    uint16_t *fb = malloc(PIXELS * sizeof(uint16_t));
    for (int i = 0; i < PIXELS; i++) {
        fb[i] = GFX_KEY;
    }
    return fb;
}

static void midday(void)
{
    sky_t noon = {.sun_alt_deg = 60.0f, .sun_up = true, .progress = 0.5f, .moon_phase = 0.5f};
    light_t light;
    light_compute(&noon, &light);
    CHECK(scene_init());
    scene_relight(&light);
}

static bool is_allowed(mood_t mood, uint16_t colour)
{
    for (int i = 0; allowed[mood][i] != 0; i++) {
        if (allowed[mood][i] == colour) {
            return true;
        }
    }
    return false;
}

/* The box around everything drawn on a blank buffer. False if nothing was. */
static bool drawn_box(const uint16_t *fb, int *x0, int *y0, int *x1, int *y1)
{
    bool any = false;
    *x0 = DISPLAY_W;
    *y0 = DISPLAY_H;
    *x1 = -1;
    *y1 = -1;
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            if (fb[y * DISPLAY_W + x] == GFX_KEY) {
                continue;
            }
            any = true;
            *x0 = x < *x0 ? x : *x0;
            *y0 = y < *y0 ? y : *y0;
            *x1 = x > *x1 ? x : *x1;
            *y1 = y > *y1 ? y : *y1;
        }
    }
    return any;
}

static void test_colours_and_size(void)
{
    for (int m = 0; m < MOOD_COUNT; m++) {
        uint16_t *fb = blank();
        render_sign(fb, (mood_t)m, 100, 100);
        int x0, y0, x1, y1, used = 0;
        bool any = drawn_box(fb, &x0, &y0, &x1, &y1);
        if (m == MOOD_CALM) {
            CHECK(!any); /* calm has no sign */
            free(fb);
            continue;
        }
        CHECK(any);
        /* Inside the 7x7 sign plus one pixel of outline all round. */
        CHECK(x0 >= 99 && y0 >= 99 && x1 <= 107 && y1 <= 107);
        bool seen[6] = {false};
        for (int i = 0; i < PIXELS; i++) {
            if (fb[i] == GFX_KEY) {
                continue;
            }
            CHECK(is_allowed((mood_t)m, fb[i]));
            for (int c = 0; allowed[m][c] != 0; c++) {
                seen[c] = seen[c] || allowed[m][c] == fb[i];
            }
        }
        for (int c = 0; allowed[m][c] != 0; c++) {
            CHECK(seen[c]); /* every listed colour is really used */
            used++;
        }
        CHECK(used >= 2);
        free(fb);
    }
    /* A mood that is not one draws nothing. */
    uint16_t *fb = blank();
    int x0, y0, x1, y1;
    render_sign(fb, MOOD_COUNT, 100, 100);
    render_sign(fb, (mood_t)-1, 100, 100);
    CHECK(!drawn_box(fb, &x0, &y0, &x1, &y1));
    free(fb);
}

static void test_drawing_off_screen_is_safe(void)
{
    /* malloc'd exactly, so AddressSanitizer catches a write outside it. */
    static const int spots[][2] = {{-20, -20}, {-4, -4}, {DISPLAY_W - 3, 3}, {DISPLAY_W + 5, 3},
                                   {3, DISPLAY_H - 3}, {3, DISPLAY_H + 5}, {DISPLAY_W - 1, DISPLAY_H - 1}};
    for (int m = 1; m < MOOD_COUNT; m++) {
        uint16_t *fb = blank();
        for (unsigned i = 0; i < sizeof(spots) / sizeof(spots[0]); i++) {
            render_sign(fb, (mood_t)m, spots[i][0], spots[i][1]);
        }
        free(fb);
    }
}

/* The highest drawn row of a fish in the three columns round `mid`. */
static int fish_top(const uint16_t *fb, int mid)
{
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int x = mid - 1; x <= mid + 1; x++) {
            if (fb[y * DISPLAY_W + x] != GFX_KEY) {
                return y;
            }
        }
    }
    return -1;
}

static void test_sign_sits_on_the_head(void)
{
    midday();
    const int cx = 160, cy = 120;
    for (int s = 0; s < SPECIES_COUNT; s++) {
        for (int left = 0; left < 2; left++) {
            for (int size = FISH_BACK_PX; size <= FISH_FRONT_PX; size += FISH_FRONT_PX - FISH_BACK_PX) {
                for (int m = 1; m < MOOD_COUNT; m++) {
                    int x = -999, y = -999;
                    render_sign_place((species_t)s, cx, cy, left, size, (mood_t)m, &x, &y);
                    int mid = x + SIGN_PX / 2;

                    /* On the head side, and over the fish, not beside it. */
                    CHECK(left ? mid < cx : mid > cx);
                    CHECK(mid > cx - size / 2 && mid < cx + size / 2);

                    uint16_t *fish = blank(), *sign = blank();
                    render_fish(fish, (species_t)s, cx, cy, left, size);
                    render_sign(sign, (mood_t)m, x, y);
                    int top = fish_top(fish, mid);
                    int x0, y0, x1, y1;
                    CHECK(top >= 0);
                    CHECK(drawn_box(sign, &x0, &y0, &x1, &y1));
                    CHECK(y1 == top - 1); /* its lowest row rests on the fish's top edge */
                    free(fish);
                    free(sign);
                }
            }
        }
    }
}

static void test_sign_stays_on_screen(void)
{
    static const float corners[][2] = {{SWIM_X_MIN, SWIM_Y_MIN}, {SWIM_X_MAX, SWIM_Y_MIN},
                                       {SWIM_X_MIN, SWIM_Y_MAX}, {SWIM_X_MAX, SWIM_Y_MAX}, {0, 0}, {319, 239}};
    for (unsigned c = 0; c < sizeof(corners) / sizeof(corners[0]); c++) {
        for (int s = 0; s < SPECIES_COUNT; s++) {
            for (int left = 0; left < 2; left++) {
                for (int m = 1; m < MOOD_COUNT; m++) {
                    int x = -999, y = -999;
                    render_sign_place((species_t)s, (int)corners[c][0], (int)corners[c][1], left, FISH_FRONT_PX,
                                      (mood_t)m, &x, &y);
                    CHECK(x >= 1 && y >= 1);
                    CHECK(x + SIGN_PX + 1 <= DISPLAY_W && y + SIGN_PX + 1 <= DISPLAY_H);
                }
            }
        }
    }
    /* Not a species: still somewhere on screen. */
    int x = -999, y = -999;
    render_sign_place((species_t)99, 160, 120, false, FISH_FRONT_PX, MOOD_WARY, &x, &y);
    CHECK(x >= 1 && y >= 1 && x + SIGN_PX + 1 <= DISPLAY_W && y + SIGN_PX + 1 <= DISPLAY_H);
}

static int count_colour(const uint16_t *fb, uint16_t colour)
{
    int n = 0;
    for (int i = 0; i < PIXELS; i++) {
        n += fb[i] == colour;
    }
    return n;
}

static void test_tank_draws_signs_while_they_last(void)
{
    midday();
    uint16_t *without = malloc(PIXELS * sizeof(uint16_t)), *with = malloc(PIXELS * sizeof(uint16_t));
    tank_t t;
    tank_init(&t, 1);
    for (int f = 0; f < TANK_FISH; f++) {
        place(&t, f, 40.0f + 50.0f * (float)f, 60.0f); /* open water, apart */
        t.fish[f].front = true;                        /* and a sign shows on either layer anyway */
    }
    const uint16_t pink = C565(255, 128, 190);

    t.fish[0].mood = MOOD_PLAYFUL;
    t.fish[0].sign_ticks = 0;
    render_tank(&t, without);
    CHECK(count_colour(without, pink) == 0); /* in the mood, but its two seconds are over */

    t.fish[0].sign_ticks = 10;
    render_tank(&t, with);
    CHECK(count_colour(with, pink) > 0);

    /* Only a small patch by fish 0 changed. */
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            if (with[y * DISPLAY_W + x] != without[y * DISPLAY_W + x]) {
                CHECK(x > 40 - 20 && x < 40 + 20 && y > 60 - 30 && y < 60 + 4);
            }
        }
    }

    /* A fish behind the rocks and kelp still shows its sign over them. */
    t.fish[0].front = false;
    place(&t, 0, tank_cover[0][0], tank_cover[0][1]);
    render_tank(&t, with);
    CHECK(count_colour(with, pink) > 0);

    /* The sign keeps its own colour at night. */
    sky_t night = {.sun_alt_deg = -40.0f, .sun_up = false, .progress = 0.5f, .moon_phase = 0.5f};
    light_t dark;
    light_compute(&night, &dark);
    scene_relight(&dark);
    render_tank(&t, with);
    CHECK(count_colour(with, pink) > 0);
    midday();

    free(without);
    free(with);
}

void test_signs(void)
{
    test_colours_and_size();
    test_drawing_off_screen_is_safe();
    test_sign_sits_on_the_head();
    test_sign_stays_on_screen();
    test_tank_draws_signs_while_they_last();
}
