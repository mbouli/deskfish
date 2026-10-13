#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "app.h"
#include "clock.h"
#include "display.h"
#include "sim.h"
#include "touch.h"

#define FRAME_MS 50

int main(void)
{
    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    display_init();
    touch_init();
    app_init((uint32_t)time(NULL));

    while (sim_pump()) {
        uint32_t start = clock_millis();
        app_frame(fb);
        uint32_t spent = clock_millis() - start;
        if (spent < FRAME_MS) {
            sim_sleep_ms(FRAME_MS - spent);
        }
    }

    free(fb);
    return 0;
}
