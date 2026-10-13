#include "app.h"

#include "display.h"
#include "gfx.h"
#include "touch.h"

#define SQUARE 40

static int x, y, dx, dy;

void app_init(uint32_t seed)
{
    (void)seed;
    x = 0;
    y = 0;
    dx = 3;
    dy = 2;
}

void app_frame(uint16_t *fb)
{
    x += dx;
    y += dy;
    if (x < 0 || x + SQUARE > DISPLAY_W) {
        dx = -dx;
        x += 2 * dx;
    }
    if (y < 0 || y + SQUARE > DISPLAY_H) {
        dy = -dy;
        y += 2 * dy;
    }

    gfx_fill(fb, RGB565(0, 0, 64));
    gfx_fill_rect(fb, x, y, SQUARE, SQUARE, RGB565(255, 160, 0));

    int tx, ty;
    if (touch_read(&tx, &ty)) {
        gfx_fill_rect(fb, tx - 4, ty - 4, 9, 9, RGB565(255, 255, 255));
    }

    display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
}
