#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "display.h"

/* A picture: w x h host-order RGB565 pixels, row by row. */
typedef struct {
    int w, h;
    const uint16_t *px;
} image_t;

/* In a keyed image, this colour means "transparent". */
#define GFX_KEY 0xF81F

/* All functions draw into a DISPLAY_W x DISPLAY_H buffer of host-order RGB565
 * and clip to it: nothing is ever written outside the buffer. */

void gfx_fill(uint16_t *fb, uint16_t colour);
void gfx_fill_rect(uint16_t *fb, int x, int y, int w, int h, uint16_t colour);

/* Copy an image with no transparency, its top-left corner at (x, y). */
void gfx_blit(uint16_t *fb, const image_t *img, int x, int y);

/* Copy an image, skipping GFX_KEY pixels, enlarged by a whole-number scale and
 * optionally mirrored left-to-right. A scale below 1 draws nothing. */
void gfx_blit_keyed(uint16_t *fb, const image_t *img, int x, int y, int scale, bool mirror);

/* Copy an image, skipping GFX_KEY pixels, stretched or shrunk to exactly
 * out_w x out_h and optionally mirrored. A size below 1 draws nothing. */
void gfx_blit_fit(uint16_t *fb, const image_t *img, int x, int y, int out_w, int out_h, bool mirror);
