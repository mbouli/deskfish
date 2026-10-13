#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "clock.h"
#include "display.h"
#include "sim.h"
#include "touch.h"

#define FRAME_MS 50

/* Write one frame as a PPM image, with no window. */
static int write_snapshot(const char *path, const uint16_t *fb)
{
    FILE *out = fopen(path, "wb");
    if (out == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }
    fprintf(out, "P6\n%d %d\n255\n", DISPLAY_W, DISPLAY_H);
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        uint16_t p = fb[i];
        unsigned char rgb[3] = {
            (unsigned char)((p >> 11) * 255 / 31),
            (unsigned char)(((p >> 5) & 0x3F) * 255 / 63),
            (unsigned char)((p & 0x1F) * 255 / 31),
        };
        fwrite(rgb, 1, 3, out);
    }
    fclose(out);
    return 0;
}

int main(int argc, char **argv)
{
    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (argc == 3 && strcmp(argv[1], "--snapshot") == 0) {
        app_init(1);
        app_draw(fb);
        int result = write_snapshot(argv[2], fb);
        free(fb);
        return result;
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
