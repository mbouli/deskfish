#include "render.h"

#include <stddef.h>

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
            gfx_fill_rect(fb, (int)(p->x + 0.5f) - 1, (int)(p->y + 0.5f) - 1, 3, 3, scene_pellet());
        }
    }
}
