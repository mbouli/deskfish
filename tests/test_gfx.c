#include <stdlib.h>

#include "gfx.h"
#include "test.h"

#define PIXELS (DISPLAY_W * DISPLAY_H)

static uint16_t *new_fb(uint16_t colour)
{
    /* Exactly the screen size, so AddressSanitizer catches any stray write. */
    uint16_t *fb = malloc(PIXELS * sizeof(uint16_t));
    for (int i = 0; i < PIXELS; i++) {
        fb[i] = colour;
    }
    return fb;
}

static int count(const uint16_t *fb, uint16_t colour)
{
    int n = 0;
    for (int i = 0; i < PIXELS; i++) {
        n += fb[i] == colour;
    }
    return n;
}

static void test_fill(void)
{
    uint16_t *fb = new_fb(0x1111);
    gfx_fill(fb, 0x2222);
    CHECK(count(fb, 0x2222) == PIXELS);
    free(fb);
}

static void test_fill_rect(void)
{
    uint16_t *fb = new_fb(0);

    gfx_fill_rect(fb, 10, 20, 5, 4, 0xFFFF);
    CHECK(count(fb, 0xFFFF) == 20);
    CHECK(fb[20 * DISPLAY_W + 10] == 0xFFFF);
    CHECK(fb[23 * DISPLAY_W + 14] == 0xFFFF);
    CHECK(fb[20 * DISPLAY_W + 9] == 0);
    CHECK(fb[24 * DISPLAY_W + 10] == 0);

    free(fb);
}

static void test_fill_rect_clips(void)
{
    uint16_t *fb = new_fb(0);

    gfx_fill_rect(fb, -20, -20, 40, 40, 0xFFFF);                       /* top-left corner */
    CHECK(count(fb, 0xFFFF) == 20 * 20);
    gfx_fill_rect(fb, DISPLAY_W - 20, DISPLAY_H - 20, 40, 40, 0xAAAA); /* bottom-right */
    CHECK(count(fb, 0xAAAA) == 20 * 20);
    gfx_fill_rect(fb, -1000, 100, 5000, 2, 0x5555);                    /* wider than the screen */
    CHECK(count(fb, 0x5555) == DISPLAY_W * 2);

    int before = count(fb, 0);
    gfx_fill_rect(fb, DISPLAY_W + 5, DISPLAY_H + 5, 10, 10, 0x1234);   /* fully off-screen */
    gfx_fill_rect(fb, -50, -50, 10, 10, 0x1234);
    gfx_fill_rect(fb, 10, 10, 0, 10, 0x1234);                          /* empty */
    gfx_fill_rect(fb, 10, 10, 10, -3, 0x1234);
    CHECK(count(fb, 0) == before);
    CHECK(count(fb, 0x1234) == 0);

    free(fb);
}

void test_gfx(void)
{
    test_fill();
    test_fill_rect();
    test_fill_rect_clips();
}
