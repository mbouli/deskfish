#include "render.h"

#include <stddef.h>
#include <string.h>

#include "assets.h"
#include "display.h"
#include "gfx.h"
#include "scene.h"
#include "tuning.h"

void render_background(uint16_t *fb)
{
    gfx_blit(fb, scene_backdrop(), 0, 0);
}

void render_fish(uint16_t *fb, species_t s, int cx, int cy, bool facing_left, int size_px)
{
    const image_t *sprite = scene_fish(s);
    if (sprite == NULL) {
        return;
    }
    gfx_blit_fit(fb, sprite, cx - size_px / 2, cy - size_px / 2, size_px, size_px, facing_left);
}

void render_foreground(uint16_t *fb)
{
    gfx_blit_keyed(fb, scene_front(), 0, DISPLAY_H - scene_front()->h, 1, false);
}

/* Nearer fish look a little bigger. The size follows `depth`, which eases
 * between layers, so a fish never jumps in size. */
static int fish_size(const fish_t *f)
{
    return FISH_BACK_PX + (int)((float)(FISH_FRONT_PX - FISH_BACK_PX) * f->depth + 0.5f);
}

static void draw_fish_layer(const tank_t *t, bool front, uint16_t *fb)
{
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &t->fish[i];
        if (f->front != front) {
            continue;
        }
        render_fish(fb, f->species, (int)(f->x + 0.5f), (int)(f->y + 0.5f), f->facing_left, fish_size(f));
    }
}

/* ---- Mood signs -------------------------------------------------------- */

#define SIGN_OUTLINE RGB565(20, 28, 60)

/* A sign is seven rows of seven characters: '.' is empty, and 'a' to 'd' pick
 * one of its colours. Signs are not lit by the scene, so they read the same
 * by day and by night. */
typedef struct {
    bool outline;       /* a dark edge round every pixel */
    uint16_t colour[4]; /* for the letters a, b, c and d */
    const char *rows[SIGN_PX];
} sign_t;

static const sign_t SIGNS[MOOD_COUNT] = {
    [MOOD_EAGER] = {false, /* a firework: too fine for an outline */
                    {RGB565(255, 214, 64), RGB565(255, 150, 40)},
                    {"...b...",
                     ".b.a.b.",
                     "..a.a..",
                     "ba...ab",
                     "..a.a..",
                     ".b.a.b.",
                     "...b..."}},
    [MOOD_WARY] = {true,
                   {RGB565(255, 128, 48)},
                   {"...a...",
                    "...a...",
                    "...a...",
                    "...a...",
                    ".......",
                    "...a...",
                    "......."}},
    [MOOD_PLAYFUL] = {true, /* a music note */
                      {RGB565(255, 128, 190)},
                      {"...aaa.",
                       "...a.aa",
                       "...a..a",
                       "...a...",
                       ".aaa...",
                       "aaaa...",
                       ".aa...."}},
    [MOOD_SLEEPY] = {true,
                     {RGB565(190, 220, 255)},
                     {"aaaa...",
                      "..a....",
                      ".a.....",
                      "aaaa...",
                      "....aaa",
                      ".....a.",
                      "....aaa"}},
    [MOOD_SULKY] = {true, /* a rain cloud */
                    {RGB565(196, 188, 216), RGB565(150, 140, 180), RGB565(108, 98, 140), RGB565(96, 176, 255)},
                    {"..aa...",
                     ".abbaa.",
                     "abbbbba",
                     "bcbbcbb",
                     ".ccccc.",
                     "d..d..d",
                     ".d..d.."}},
};

static const sign_t *sign_for(mood_t mood)
{
    if ((int)mood <= MOOD_CALM || (int)mood >= MOOD_COUNT) {
        return NULL; /* calm has no sign */
    }
    return &SIGNS[mood];
}

static void put(uint16_t *fb, int x, int y, uint16_t colour)
{
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H) {
        fb[y * DISPLAY_W + x] = colour;
    }
}

void render_sign(uint16_t *fb, mood_t mood, int x, int y)
{
    const sign_t *s = sign_for(mood);
    if (s == NULL) {
        return;
    }
    if (s->outline) { /* the edge first, then the sign over it */
        for (int j = 0; j < SIGN_PX; j++) {
            for (int i = 0; i < SIGN_PX; i++) {
                if (s->rows[j][i] == '.') {
                    continue;
                }
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        put(fb, x + i + dx, y + j + dy, SIGN_OUTLINE);
                    }
                }
            }
        }
    }
    for (int j = 0; j < SIGN_PX; j++) {
        for (int i = 0; i < SIGN_PX; i++) {
            char c = s->rows[j][i];
            if (c != '.') {
                put(fb, x + i, y + j, s->colour[c - 'a']);
            }
        }
    }
}

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

void render_sign_place(species_t s, int cx, int cy, bool facing_left, int size_px, mood_t mood, int *x, int *y)
{
    int left = cx - size_px / 2, top = cy - size_px / 2;
    /* Over the head: a little forward of the middle. */
    int mid = cx + (facing_left ? -1 : 1) * (size_px * 3 / 16);

    /* Find the fish's top edge there, reading its sprite exactly as
     * gfx_blit_fit will draw it. The unlit sprite has the same shape. */
    int edge = top;
    if ((int)s >= 0 && (int)s < SPECIES_COUNT && size_px >= 1) {
        const image_t *sprite = &asset_fish[s];
        bool found = false;
        for (int row = 0; row < size_px && !found; row++) {
            int sy = row * sprite->h / size_px;
            for (int col = mid - 1; col <= mid + 1 && !found; col++) {
                if (col < left || col >= left + size_px) {
                    continue;
                }
                int sx = (col - left) * sprite->w / size_px;
                if (facing_left) {
                    sx = sprite->w - 1 - sx;
                }
                if (sprite->px[sy * sprite->w + sx] != GFX_KEY) {
                    edge = top + row;
                    found = true;
                }
            }
        }
    }

    /* The sign's lowest drawn row, counting its outline, rests on that edge. */
    int lowest = SIGN_PX - 1;
    const sign_t *sign = sign_for(mood);
    if (sign != NULL) {
        while (lowest > 0 && strspn(sign->rows[lowest], ".") == SIGN_PX) {
            lowest--;
        }
        lowest += sign->outline ? 1 : 0;
    }
    *x = clamp(mid - SIGN_PX / 2, 1, DISPLAY_W - SIGN_PX - 1);
    *y = clamp(edge - 1 - lowest, 1, DISPLAY_H - SIGN_PX - 1);
}

void render_tank(const tank_t *t, uint16_t *fb)
{
    /* Back fish, then the rocks and kelp, then front fish. */
    render_background(fb);
    draw_fish_layer(t, false, fb);
    render_foreground(fb);
    draw_fish_layer(t, true, fb);
    for (int i = 0; i < TANK_PELLETS; i++) {
        const pellet_t *p = &t->pellets[i];
        if (p->alive) {
            gfx_fill_rect(fb, (int)(p->x + 0.5f) - 1, (int)(p->y + 0.5f) - 1, 3, 3, scene_pellet());
        }
    }
    /* Signs go on last, so the rocks and kelp never hide one. */
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &t->fish[i];
        if (f->sign_ticks > 0) {
            int x, y;
            render_sign_place(f->species, (int)(f->x + 0.5f), (int)(f->y + 0.5f), f->facing_left, fish_size(f),
                              f->mood, &x, &y);
            render_sign(fb, f->mood, x, y);
        }
    }
}
