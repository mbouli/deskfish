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

/* A 3x2 picture:  A B K
 *                 C K D     (K is the key colour) */
#define A 0x0001
#define B 0x0002
#define C 0x0003
#define D 0x0004
static const uint16_t tiny_px[6] = {A, B, GFX_KEY, C, GFX_KEY, D};
static const image_t tiny = {3, 2, tiny_px};

static uint16_t at(const uint16_t *fb, int x, int y)
{
    return fb[y * DISPLAY_W + x];
}

static void test_blit_opaque(void)
{
    uint16_t *fb = new_fb(0x7777);
    gfx_blit(fb, &tiny, 10, 10);
    CHECK(at(fb, 10, 10) == A);
    CHECK(at(fb, 12, 10) == GFX_KEY); /* opaque blit copies every pixel */
    CHECK(at(fb, 12, 11) == D);
    CHECK(count(fb, 0x7777) == PIXELS - 6);

    gfx_blit(fb, &tiny, -1, -1); /* only D lands, at 1,0 */
    CHECK(at(fb, 1, 0) == D);
    CHECK(at(fb, 0, 0) == GFX_KEY);
    gfx_blit(fb, &tiny, DISPLAY_W - 1, DISPLAY_H - 1); /* only A lands */
    CHECK(at(fb, DISPLAY_W - 1, DISPLAY_H - 1) == A);
    gfx_blit(fb, &tiny, DISPLAY_W, 0); /* fully off: no crash */
    gfx_blit(fb, &tiny, 0, -2);
    free(fb);
}

static void test_blit_keyed(void)
{
    uint16_t *fb = new_fb(0x7777);
    gfx_blit_keyed(fb, &tiny, 10, 10, 1, false);
    CHECK(at(fb, 10, 10) == A);
    CHECK(at(fb, 11, 10) == B);
    CHECK(at(fb, 12, 10) == 0x7777); /* key skipped */
    CHECK(at(fb, 10, 11) == C);
    CHECK(at(fb, 11, 11) == 0x7777);
    CHECK(at(fb, 12, 11) == D);
    CHECK(count(fb, 0x7777) == PIXELS - 4);
    free(fb);
}

static void test_blit_keyed_mirror(void)
{
    uint16_t *fb = new_fb(0x7777);
    gfx_blit_keyed(fb, &tiny, 10, 10, 1, true); /* rows become  K B A  /  D K C */
    CHECK(at(fb, 10, 10) == 0x7777);
    CHECK(at(fb, 11, 10) == B);
    CHECK(at(fb, 12, 10) == A);
    CHECK(at(fb, 10, 11) == D);
    CHECK(at(fb, 12, 11) == C);
    free(fb);
}

static void test_blit_keyed_scale(void)
{
    uint16_t *fb = new_fb(0x7777);
    gfx_blit_keyed(fb, &tiny, 10, 10, 2, false); /* each pixel becomes 2x2 */
    CHECK(at(fb, 10, 10) == A);
    CHECK(at(fb, 11, 11) == A);
    CHECK(at(fb, 12, 10) == B);
    CHECK(at(fb, 14, 10) == 0x7777);
    CHECK(at(fb, 10, 12) == C);
    CHECK(at(fb, 15, 13) == D);
    CHECK(count(fb, 0x7777) == PIXELS - 16);

    int before = count(fb, 0x7777);
    gfx_blit_keyed(fb, &tiny, 50, 50, 0, false); /* nonsense scales draw nothing */
    gfx_blit_keyed(fb, &tiny, 50, 50, -3, false);
    CHECK(count(fb, 0x7777) == before);
    free(fb);
}

static void test_blit_keyed_clips(void)
{
    uint16_t *fb = new_fb(0x7777);
    /* Hanging off each edge and corner at 2x, plus fully off-screen. Passing
     * means AddressSanitizer stays quiet and only on-screen pixels change. */
    gfx_blit_keyed(fb, &tiny, -3, -2, 2, false);
    gfx_blit_keyed(fb, &tiny, DISPLAY_W - 2, 5, 2, true);
    gfx_blit_keyed(fb, &tiny, 5, DISPLAY_H - 1, 2, false);
    gfx_blit_keyed(fb, &tiny, DISPLAY_W - 1, DISPLAY_H - 1, 2, true);
    gfx_blit_keyed(fb, &tiny, -100, -100, 2, false);
    gfx_blit_keyed(fb, &tiny, DISPLAY_W + 10, DISPLAY_H + 10, 2, false);
    /* At -3,-2 with scale 2, screen 0,0 samples source (1,1), which is K;
     * screen 1,0 samples source (2,1), which is D. */
    CHECK(at(fb, 0, 0) == 0x7777);
    CHECK(at(fb, 1, 0) == D);
    free(fb);
}

void test_gfx(void)
{
    test_fill();
    test_fill_rect();
    test_fill_rect_clips();
    test_blit_opaque();
    test_blit_keyed();
    test_blit_keyed_mirror();
    test_blit_keyed_scale();
    test_blit_keyed_clips();
}
