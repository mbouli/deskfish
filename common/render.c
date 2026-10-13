#include "render.h"

#include "assets.h"
#include "display.h"
#include "gfx.h"
#include "tuning.h"

/* The art is shorter than the screen: it sits at the bottom, under plain water. */
static int art_top(void)
{
    return DISPLAY_H - asset_back.h;
}

void render_background(uint16_t *fb)
{
    gfx_fill_rect(fb, 0, 0, DISPLAY_W, art_top(), asset_water);
    gfx_blit(fb, &asset_back, 0, art_top());
}

void render_fish(uint16_t *fb, species_t s, int cx, int cy, bool facing_left, int size_px)
{
    if ((int)s < 0 || (int)s >= SPECIES_COUNT) {
        return;
    }
    gfx_blit_fit(fb, &asset_fish[s], cx - size_px / 2, cy - size_px / 2, size_px, size_px, facing_left);
}

void render_foreground(uint16_t *fb)
{
    gfx_blit_keyed(fb, &asset_front, 0, art_top(), 1, false);
}

#define PELLET_COLOUR RGB565(214, 170, 96)

static void draw_fish_layer(const tank_t *t, bool front, uint16_t *fb)
{
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &t->fish[i];
        if (f->front != front) {
            continue;
        }
        /* Nearer fish look a little bigger. The size follows `depth`, which
         * eases between layers, so a fish never jumps in size. */
        int size = FISH_BACK_PX + (int)((float)(FISH_FRONT_PX - FISH_BACK_PX) * f->depth + 0.5f);
        render_fish(fb, f->species, (int)(f->x + 0.5f), (int)(f->y + 0.5f), f->facing_left, size);
    }
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
            gfx_fill_rect(fb, (int)(p->x + 0.5f) - 1, (int)(p->y + 0.5f) - 1, 3, 3, PELLET_COLOUR);
        }
    }
}
