/* Stand-ins for the three platform interfaces, so app.c can be tested. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "clock.h"
#include "display.h"
#include "touch.h"

uint32_t stub_millis;
int stub_minutes = 12 * 60;
int stub_pushes;
bool stub_touch_down;
int stub_touch_x, stub_touch_y;

uint32_t clock_millis(void)
{
    return stub_millis;
}

int clock_minutes(void)
{
    return stub_minutes;
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
