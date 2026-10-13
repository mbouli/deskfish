/* Stand-ins for the three platform interfaces, so app.c can be tested. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "clock.h"
#include "display.h"
#include "touch.h"

uint32_t stub_millis;
int stub_pushes;

uint32_t clock_millis(void)
{
    return stub_millis;
}

int clock_minutes(void)
{
    return 12 * 60;
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
    (void)x;
    (void)y;
    return false;
}
