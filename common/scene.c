#include "scene.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "assets.h"
#include "display.h"

#define FRONT_H 192
#define ART_TOP (DISPLAY_H - FRONT_H) /* the art sits at the bottom of the screen */
#define SQUASH 1.35f                  /* vertical distances count for more: rings become layers */
#define SPRITE_PX (16 * 16)

#define SUN_RADIUS 11.0f
#define SUN_CORE_RADIUS 8.0f
#define MOON_RADIUS 10.0f
#define STAR_MIN_BAND 4               /* stars only show in the darker half of the bands */

#define SUN_COLOUR RGB565(255, 246, 214)
#define SUN_CORE RGB565(255, 255, 240)
#define MOON_COLOUR RGB565(228, 233, 248)
#define STAR_COLOUR RGB565(206, 214, 246)
#define STAR_ARM RGB565(150, 160, 214)
#define PELLET_COLOUR RGB565(214, 170, 96)

/* The distant ruins are the sky behind them times this, per channel. At midday
 * it turns the art's water colour into the art's own silhouette colour. */
static const float RUIN_SHADE[3] = {0.84f, 0.90f, 0.97f};
static const float HALO_RADIUS[LIGHT_HALOS] = {26.0f, 64.0f, 120.0f};

static uint16_t *backdrop_px;
static uint16_t *front_px;
static uint16_t fish_px[SPECIES_COUNT][SPRITE_PX];
static image_t backdrop, front, fish[SPECIES_COUNT];
static uint16_t pellet = PELLET_COLOUR;
static unsigned relights;

/* How far each band's edge is pushed in or out on each row, in pixels. Made
 * once, so edges look hand-drawn and never shimmer. */
static signed char wobble[DISPLAY_H][LIGHT_BANDS - 1];

static struct {
    short x, y;
    bool cross; /* drawn with four dimmer arms */
} stars[LIGHT_MAX_STARS];

bool scene_init(void)
{
    if (backdrop_px != NULL) {
        return true;
    }
    backdrop_px = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    front_px = malloc(DISPLAY_W * FRONT_H * sizeof(uint16_t));
    if (backdrop_px == NULL || front_px == NULL) {
        free(backdrop_px);
        free(front_px);
        backdrop_px = NULL;
        front_px = NULL;
        return false;
    }
    backdrop = (image_t){DISPLAY_W, DISPLAY_H, backdrop_px};
    front = (image_t){DISPLAY_W, FRONT_H, front_px};
    for (int s = 0; s < SPECIES_COUNT; s++) {
        fish[s] = (image_t){16, 16, fish_px[s]};
    }

    /* Streaks: rows are grouped in twos and threes so the steps are chunky,
     * and every shift is a multiple of 2 pixels. */
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int i = 0; i < LIGHT_BANDS - 1; i++) {
            float w = 4.0f * sinf((float)(y / 3) * 0.9f + (float)i * 1.9f) +
                      2.0f * sinf((float)(y / 2) * 2.3f + (float)i);
            wobble[y][i] = (signed char)(2 * (int)lroundf(w));
        }
    }

    /* The same stars every night, from a fixed seed. */
    uint32_t r = 0x51A25EEDu;
    for (int i = 0; i < LIGHT_MAX_STARS; i++) {
        r = r * 1664525u + 1013904223u;
        stars[i].x = (short)(2 + (r >> 8) % (DISPLAY_W - 4));
        r = r * 1664525u + 1013904223u;
        stars[i].y = (short)(2 + (r >> 8) % 148);
        r = r * 1664525u + 1013904223u;
        stars[i].cross = (r >> 8) % 100 < 18;
    }

    /* Start out as the plain art in daylight, so there is always a picture. */
    sky_t noon = {.sun_alt_deg = 60.0f, .sun_up = true, .progress = 0.5f, .moon_phase = 0.5f};
    light_t light;
    light_compute(&noon, &light);
    scene_relight(&light);
    return true;
}

/* Multiply a packed colour's red, green and blue by three factors. The result
 * is never the transparent key. */
static uint16_t tint(uint16_t colour, const float t[3])
{
    int r = (int)((float)(colour >> 11) * t[0] + 0.5f);
    int g = (int)((float)((colour >> 5) & 0x3F) * t[1] + 0.5f);
    int b = (int)((float)(colour & 0x1F) * t[2] + 0.5f);
    r = r > 31 ? 31 : r;
    g = g > 63 ? 63 : g;
    b = b > 31 ? 31 : b;
    uint16_t out = (uint16_t)((r << 11) | (g << 5) | b);
    return out == GFX_KEY ? (uint16_t)(GFX_KEY - 1) : out;
}

static int row_wobble(int y, int i)
{
    return y >= 0 && y < DISPLAY_H ? wobble[y][i] : 0;
}

/* Which band of sky a pixel is in: 0 nearest the sun, LIGHT_BANDS - 1 the rest. */
static int band_at(const light_t *light, int x, int y)
{
    float dx = (float)(x - light->centre_x), dy = (float)(y - light->centre_y) * SQUASH;
    float r = sqrtf(dx * dx + dy * dy);
    for (int i = 0; i < LIGHT_BANDS - 1; i++) {
        if (r + (float)row_wobble(y, i) < (float)light->band_radius[i]) {
            return i;
        }
    }
    return LIGHT_BANDS - 1;
}

static void put(int x, int y, uint16_t colour)
{
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H) {
        backdrop_px[y * DISPLAY_W + x] = colour;
    }
}

static void draw_sky(const light_t *light)
{
    for (int y = 0; y < DISPLAY_H; y++) {
        for (int x = 0; x < DISPLAY_W; x++) {
            int band = band_at(light, x, y);
            uint16_t colour = light->band_colour[band];
            if (light->show_halo && band == LIGHT_BANDS - 1) {
                /* The moon's rings only ever replace the outermost sky colour. */
                float dx = (float)(x - light->moon_x), dy = (float)(y - light->moon_y) * SQUASH;
                float r = sqrtf(dx * dx + dy * dy);
                for (int k = 0; k < LIGHT_HALOS; k++) {
                    if (r + (float)row_wobble(y, k) < HALO_RADIUS[k]) {
                        colour = light->halo_colour[k];
                        break;
                    }
                }
            }
            backdrop_px[y * DISPLAY_W + x] = colour;
        }
    }
}

static void draw_stars(const light_t *light)
{
    int count = light->star_count < 0 ? 0 : light->star_count > LIGHT_MAX_STARS ? LIGHT_MAX_STARS : light->star_count;
    for (int i = 0; i < count; i++) {
        int x = stars[i].x, y = stars[i].y;
        if (band_at(light, x, y) < STAR_MIN_BAND) {
            continue; /* washed out by the glow near the sun */
        }
        if (stars[i].cross) {
            put(x - 1, y, STAR_ARM);
            put(x + 1, y, STAR_ARM);
            put(x, y - 1, STAR_ARM);
            put(x, y + 1, STAR_ARM);
        }
        put(x, y, STAR_COLOUR);
    }
}

static void draw_disc(int cx, int cy, float radius, uint16_t colour)
{
    int reach = (int)radius + 1;
    for (int y = cy - reach; y <= cy + reach; y++) {
        for (int x = cx - reach; x <= cx + reach; x++) {
            float dx = (float)(x - cx), dy = (float)(y - cy);
            if (dx * dx + dy * dy < radius * radius) {
                put(x, y, colour);
            }
        }
    }
}

/* The lit part of the moon. The boundary between light and dark is the disc's
 * own edge squashed sideways by the cosine of the phase: at a quarter it is a
 * straight line down the middle, at full it is the far edge. */
static void draw_moon(const light_t *light)
{
    const float pi = 3.14159265f;
    float squash = cosf(2.0f * pi * light->moon_phase); /* 1 at new, 0 at quarters, -1 at full */
    bool waxing = light->moon_phase < 0.5f;
    int reach = (int)MOON_RADIUS + 1;
    for (int y = light->moon_y - reach; y <= light->moon_y + reach; y++) {
        for (int x = light->moon_x - reach; x <= light->moon_x + reach; x++) {
            float dx = (float)(x - light->moon_x), dy = (float)(y - light->moon_y);
            float inside = MOON_RADIUS * MOON_RADIUS - dx * dx - dy * dy;
            if (inside <= 0.0f) {
                continue;
            }
            float terminator = squash * sqrtf(MOON_RADIUS * MOON_RADIUS - dy * dy);
            bool is_lit = waxing ? dx > terminator : dx < -terminator;
            if (is_lit) {
                put(x, y, MOON_COLOUR);
            }
        }
    }
}

static void draw_ruins_and_sand(const light_t *light)
{
    for (int y = 0; y < FRONT_H; y++) {
        uint16_t *row = &backdrop_px[(y + ART_TOP) * DISPLAY_W];
        const uint16_t *sand = &asset_sand.px[y * DISPLAY_W];
        for (int x = 0; x < DISPLAY_W; x++) {
            if (sand[x] != GFX_KEY) {
                row[x] = tint(sand[x], light->tint);
            } else if ((asset_far_mask[y][x >> 3] >> (x & 7)) & 1) {
                row[x] = tint(row[x], RUIN_SHADE); /* the sky behind, a little darker */
            }
        }
    }
}

static void tint_keyed(uint16_t *dst, const uint16_t *src, int count, const float t[3])
{
    for (int i = 0; i < count; i++) {
        dst[i] = src[i] == GFX_KEY ? GFX_KEY : tint(src[i], t);
    }
}

void scene_relight(const light_t *light)
{
    if (backdrop_px == NULL) {
        return;
    }
    draw_sky(light);
    draw_stars(light);
    if (light->show_sun) {
        draw_disc(light->centre_x, light->centre_y, SUN_RADIUS, SUN_COLOUR);
        draw_disc(light->centre_x, light->centre_y, SUN_CORE_RADIUS, SUN_CORE);
    }
    if (light->show_moon) {
        draw_moon(light);
    }
    draw_ruins_and_sand(light);

    tint_keyed(front_px, asset_front.px, DISPLAY_W * FRONT_H, light->tint);
    for (int s = 0; s < SPECIES_COUNT; s++) {
        tint_keyed(fish_px[s], asset_fish[s].px, SPRITE_PX, light->tint);
    }
    pellet = tint(PELLET_COLOUR, light->tint);
    relights++;
}

const image_t *scene_backdrop(void)
{
    return &backdrop;
}

const image_t *scene_front(void)
{
    return &front;
}

const image_t *scene_fish(species_t s)
{
    if ((int)s < 0 || (int)s >= SPECIES_COUNT) {
        return NULL;
    }
    return &fish[s];
}

uint16_t scene_pellet(void)
{
    return pellet;
}

unsigned scene_relight_count(void)
{
    return relights;
}
