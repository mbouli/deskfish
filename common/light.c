#include "light.h"

#include <math.h>

#include "display.h"

#define PI 3.14159265f

/* The sun and moon travel an arc: in at the left edge, out at the right, low
 * at both ends and high in the middle. */
#define EDGE_X 20
#define SPAN_X 280
#define HORIZON_Y 128
#define ARC_HEIGHT 106
#define SINK_PX_PER_DEG 4.0f /* how fast the bands drop once the sun is below the horizon */

#define SUNRISE_ALT (-0.833f)
#define FISH_NIGHT_ALT (-6.0f)   /* the end of civil twilight */
#define HALO_FULL_ALT (-12.0f)   /* the moon's halo fades in between these two heights */
#define STARS_START_ALT (-4.0f)
#define STARS_FULL_ALT (-12.0f)
#define MOONLIGHT 0.08f          /* how much a full moon lifts the night tint */

/* The look at one sun height. In between, each number is mixed with its
 * neighbour's. Colours are red, green, blue, 0..255. */
typedef struct {
    float alt;
    short radius[LIGHT_BANDS - 1];
    unsigned char colour[LIGHT_BANDS][3];
    float tint[3];
} key_t;

#define NIGHT_SKY {14, 18, 52}

static const key_t KEYS[] = {
    {   /* night: one flat dark sky; the moon's halo does the rest */
        -12.0f,
        {18, 38, 70, 112, 160, 214, 272},
        {NIGHT_SKY, NIGHT_SKY, NIGHT_SKY, NIGHT_SKY, NIGHT_SKY, NIGHT_SKY, NIGHT_SKY, NIGHT_SKY},
        {0.34f, 0.38f, 0.60f},
    },
    {   /* dusk: an afterglow where the sun went down */
        -4.0f,
        {18, 38, 70, 112, 160, 214, 272},
        {{236, 128, 98}, {226, 110, 96}, {170, 74, 118}, {108, 62, 138},
         {62, 50, 124}, {34, 36, 92}, {24, 28, 74}, {18, 22, 62}},
        {0.50f, 0.44f, 0.62f},
    },
    {   /* the sun on the horizon: yellow through red to deep blue */
        0.0f,
        {24, 50, 82, 120, 164, 214, 272},
        {{255, 236, 150}, {255, 196, 92}, {250, 146, 72}, {226, 96, 82},
         {176, 72, 120}, {112, 64, 140}, {64, 52, 128}, {36, 40, 98}},
        {0.74f, 0.60f, 0.70f},
    },
    {   /* ...held for the first two degrees, so the sunset lasts */
        2.0f,
        {24, 50, 82, 120, 164, 214, 272},
        {{255, 236, 150}, {255, 196, 92}, {250, 146, 72}, {226, 96, 82},
         {176, 72, 120}, {112, 64, 140}, {64, 52, 128}, {36, 40, 98}},
        {0.74f, 0.60f, 0.70f},
    },
    {   /* a low sun: pale gold into morning blue */
        12.0f,
        {20, 34, 52, 72, 100, 136, 180},
        {{255, 244, 190}, {236, 236, 214}, {196, 222, 246}, {160, 198, 243},
         {128, 176, 240}, {104, 158, 234}, {88, 144, 230}, {62, 121, 221}},
        {0.96f, 0.96f, 0.98f},
    },
    {   /* a high sun: the art's own blue, lighter towards the sun */
        35.0f,
        {14, 22, 34, 46, 66, 86, 140},
        {{232, 244, 255}, {214, 236, 255}, {180, 214, 250}, {150, 196, 246},
         {124, 176, 240}, {104, 158, 234}, {78, 136, 226}, {62, 121, 221}},
        {1.0f, 1.0f, 1.0f},
    },
};
#define KEY_COUNT ((int)(sizeof(KEYS) / sizeof(KEYS[0])))

static const unsigned char HALO[LIGHT_HALOS][3] = {{46, 54, 110}, {30, 36, 84}, {22, 26, 68}};

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

static float mix(float a, float b, float t)
{
    return a + (b - a) * t;
}

static uint16_t pack(float r, float g, float b)
{
    return RGB565((int)(r + 0.5f), (int)(g + 0.5f), (int)(b + 0.5f));
}

void light_compute(const sky_t *sky, light_t *out)
{
    float alt = sky->sun_alt_deg;

    /* Which two keyframes are we between, and how far along? */
    int hi = 1;
    while (hi < KEY_COUNT - 1 && alt > KEYS[hi].alt) {
        hi++;
    }
    const key_t *a = &KEYS[hi - 1], *b = &KEYS[hi];
    float t = clamp01((alt - a->alt) / (b->alt - a->alt));

    float outer[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < LIGHT_BANDS; i++) {
        float r = mix(a->colour[i][0], b->colour[i][0], t);
        float g = mix(a->colour[i][1], b->colour[i][1], t);
        float bl = mix(a->colour[i][2], b->colour[i][2], t);
        out->band_colour[i] = pack(r, g, bl);
        if (i == LIGHT_BANDS - 1) {
            outer[0] = r;
            outer[1] = g;
            outer[2] = bl;
        }
    }
    for (int i = 0; i < LIGHT_BANDS - 1; i++) {
        out->band_radius[i] = (int)(mix((float)a->radius[i], (float)b->radius[i], t) + 0.5f);
    }
    for (int c = 0; c < 3; c++) {
        out->tint[c] = mix(a->tint[c], b->tint[c], t);
    }

    /* How dark it is, 0 at the end of civil twilight to 1 at full night. */
    float darkness = clamp01((FISH_NIGHT_ALT - alt) / (FISH_NIGHT_ALT - HALO_FULL_ALT));

    out->show_sun = alt > SUNRISE_ALT;
    out->show_moon = !out->show_sun;
    out->moon_phase = sky->moon_phase;
    out->fish_night = alt < FISH_NIGHT_ALT;

    float arc = sinf(PI * clamp01(sky->progress));
    if (out->show_sun) {
        out->centre_x = EDGE_X + (int)((float)SPAN_X * clamp01(sky->progress) + 0.5f);
        out->centre_y = HORIZON_Y - (int)((float)ARC_HEIGHT * arc + 0.5f);
        out->moon_x = out->centre_x;
        out->moon_y = out->centre_y;
    } else {
        /* The bands stay where the sun set (the first half of the night) or
         * where it will rise (the second half), and sink as it goes lower. */
        out->centre_x = sky->progress < 0.5f ? EDGE_X + SPAN_X : EDGE_X;
        out->centre_y = HORIZON_Y + (int)((SUNRISE_ALT - alt) * SINK_PX_PER_DEG + 0.5f);
        out->moon_x = EDGE_X + (int)((float)SPAN_X * clamp01(sky->progress) + 0.5f);
        out->moon_y = HORIZON_Y - (int)((float)ARC_HEIGHT * arc + 0.5f);
    }

    /* The halo starts as the sky colour it sits on and grows into its own. */
    out->show_halo = out->show_moon && darkness > 0.0f;
    for (int i = 0; i < LIGHT_HALOS; i++) {
        out->halo_colour[i] = pack(mix(outer[0], HALO[i][0], darkness), mix(outer[1], HALO[i][1], darkness),
                                   mix(outer[2], HALO[i][2], darkness));
    }

    float starriness = clamp01((STARS_START_ALT - alt) / (STARS_START_ALT - STARS_FULL_ALT));
    out->star_count = (int)((float)LIGHT_MAX_STARS * starriness + 0.5f);

    /* A fuller moon lifts the night a little. */
    float fullness = 1.0f - fabsf(2.0f * sky->moon_phase - 1.0f);
    for (int c = 0; c < 3; c++) {
        out->tint[c] += MOONLIGHT * fullness * darkness;
    }
}
