#include "app.h"

#include "display.h"
#include "render.h"

/* A still line-up of the starting cast; Task 3 replaces this with the tank. */
static const struct {
    species_t species;
    int x, y;
    bool facing_left;
} line_up[] = {
    {SPECIES_CLOWNFISH, 160, 110, false},
    {SPECIES_GOLDFISH, 90, 90, false},
    {SPECIES_PUFFERFISH, 235, 130, true},
    {SPECIES_CATFISH, 70, 185, false},
    {SPECIES_ANGELFISH, 250, 70, true},
};

void app_init(uint32_t seed)
{
    (void)seed;
}

void app_draw(uint16_t *fb)
{
    render_background(fb);
    for (unsigned i = 0; i < sizeof(line_up) / sizeof(line_up[0]); i++) {
        render_fish(fb, line_up[i].species, line_up[i].x, line_up[i].y, line_up[i].facing_left);
    }
    render_foreground(fb);
}

void app_frame(uint16_t *fb)
{
    app_draw(fb);
    display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
}
