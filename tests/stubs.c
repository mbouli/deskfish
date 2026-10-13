/* Stand-ins for the three platform interfaces, so app.c can be tested. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "clock.h"
#include "display.h"
#include "touch.h"

uint32_t stub_millis;
int64_t stub_unix = 1782043200; /* 2026-06-21 12:00 UTC: early morning in Auburn, sun up */
int stub_pushes;
bool stub_touch_down;
int stub_touch_x, stub_touch_y;

uint32_t clock_millis(void)
{
    return stub_millis;
}

int64_t clock_unix(void)
{
    return stub_unix;
}

void display_init(void)
{
}

void display_push(const uint16_t *pixels, int x, int y, int w, int h)
{
    (void)pixels;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    stub_pushes++;
}

bool touch_init(void)
{
    return true;
}

bool touch_read(int *x, int *y)
{
    if (!stub_touch_down) {
        return false;
    }
    *x = stub_touch_x;
    *y = stub_touch_y;
    return true;
}
