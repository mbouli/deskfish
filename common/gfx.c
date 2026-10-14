#include "gfx.h"

#include <string.h>

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

void gfx_blit(uint16_t *fb, const image_t *img, int x, int y)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + img->w > DISPLAY_W ? DISPLAY_W : x + img->w;
    int y1 = y + img->h > DISPLAY_H ? DISPLAY_H : y + img->h;
    if (x0 >= x1) {
        return;
    }
    for (int row = y0; row < y1; row++) {
        memcpy(&fb[row * DISPLAY_W + x0], &img->px[(row - y) * img->w + (x0 - x)],
               (size_t)(x1 - x0) * sizeof(uint16_t));
    }
}

void gfx_blit_keyed(uint16_t *fb, const image_t *img, int x, int y, int scale, bool mirror)
{
    if (scale < 1) {
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + img->w * scale > DISPLAY_W ? DISPLAY_W : x + img->w * scale;
    int y1 = y + img->h * scale > DISPLAY_H ? DISPLAY_H : y + img->h * scale;
    for (int row = y0; row < y1; row++) {
        const uint16_t *src = &img->px[((row - y) / scale) * img->w];
        uint16_t *dst = &fb[row * DISPLAY_W];
        for (int col = x0; col < x1; col++) {
            int sx = (col - x) / scale;
            if (mirror) {
                sx = img->w - 1 - sx;
            }
            uint16_t pixel = src[sx];
            if (pixel != GFX_KEY) {
                dst[col] = pixel;
            }
        }
    }
}

void gfx_blit_fit(uint16_t *fb, const image_t *img, int x, int y, int out_w, int out_h, bool mirror)
{
    if (out_w < 1 || out_h < 1) {
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + out_w > DISPLAY_W ? DISPLAY_W : x + out_w;
    int y1 = y + out_h > DISPLAY_H ? DISPLAY_H : y + out_h;
    for (int row = y0; row < y1; row++) {
        const uint16_t *src = &img->px[((row - y) * img->h / out_h) * img->w];
        uint16_t *dst = &fb[row * DISPLAY_W];
        for (int col = x0; col < x1; col++) {
            int sx = (col - x) * img->w / out_w;
            if (mirror) {
                sx = img->w - 1 - sx;
            }
            uint16_t pixel = src[sx];
            if (pixel != GFX_KEY) {
                dst[col] = pixel;
            }
        }
    }
}
