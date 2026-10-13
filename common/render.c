#include "render.h"

#include "assets.h"
#include "display.h"
#include "gfx.h"

#define FISH_SCALE 2

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

void render_fish(uint16_t *fb, species_t s, int cx, int cy, bool facing_left)
{
    if ((int)s < 0 || (int)s >= SPECIES_COUNT) {
        return;
    }
    const image_t *sprite = &asset_fish[s];
    gfx_blit_keyed(fb, sprite, cx - sprite->w * FISH_SCALE / 2, cy - sprite->h * FISH_SCALE / 2,
                   FISH_SCALE, facing_left);
}

void render_foreground(uint16_t *fb)
{
    gfx_blit_keyed(fb, &asset_front, 0, art_top(), 1, false);
}

void render_tank(const tank_t *t, int minutes, uint16_t *fb)
{
    (void)minutes; /* night dimming arrives in Part 2 */
    render_background(fb);
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &t->fish[i];
        render_fish(fb, f->species, (int)(f->x + 0.5f), (int)(f->y + 0.5f), f->facing_left);
    }
    render_foreground(fb);
}
