#include "gfx.h"

void gfx_fill(uint16_t *fb, uint16_t colour)
{
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        fb[i] = colour;
    }
}

void gfx_fill_rect(uint16_t *fb, int x, int y, int w, int h, uint16_t colour)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > DISPLAY_W ? DISPLAY_W : x + w;
    int y1 = y + h > DISPLAY_H ? DISPLAY_H : y + h;
    for (int row = y0; row < y1; row++) {
        for (int col = x0; col < x1; col++) {
            fb[row * DISPLAY_W + col] = colour;
        }
    }
}
