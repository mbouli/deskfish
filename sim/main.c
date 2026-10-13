#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "clock.h"
#include "display.h"
#include "render.h"
#include "sim.h"
#include "tank.h"
#include "touch.h"

#define FRAME_MS 50
#define FAST_SPEED 10

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

static void usage(void)
{
    fprintf(stderr,
            "usage: deskfish_sim [--seed S]\n"
            "       deskfish_sim --headless [--seed S] [--ticks N] [--snapshot FILE.ppm]\n"
            "keys:  F fast-forward, R restart with new fish positions\n");
}

/* Run with no window and print where everything ended up. */
static int run_headless(uint32_t seed, long ticks, const char *snapshot, uint16_t *fb)
{
    tank_t tank;
    tank_init(&tank, seed);
    for (long i = 0; i < ticks; i++) {
        tank_tick(&tank, 12 * 60);
    }
    printf("seed %u, %u ticks\n", (unsigned)seed, (unsigned)tank.tick);
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &tank.fish[i];
        printf("  %-11s bold %d social %d  at %5.1f,%5.1f  facing %s\n", species_info(f->species)->name,
               f->bold, f->social, (double)f->x, (double)f->y, f->facing_left ? "left" : "right");
    }
    if (snapshot != NULL) {
        render_tank(&tank, 12 * 60, fb);
        return write_snapshot(snapshot, fb);
    }
    return 0;
}

int main(int argc, char **argv)
{
    uint32_t seed = (uint32_t)time(NULL);
    long ticks = 0;
    bool headless = false;
    const char *snapshot = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            ticks = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--snapshot") == 0 && i + 1 < argc) {
            snapshot = argv[++i];
        } else {
            usage();
            return 2;
        }
    }
    if (ticks < 0 || (!headless && (ticks != 0 || snapshot != NULL))) {
        usage();
        return 2;
    }

    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (headless) {
        int result = run_headless(seed, ticks, snapshot, fb);
        free(fb);
        return result;
    }

    display_init();
    touch_init();
    app_init(seed);
    printf("seed %u\n", (unsigned)seed);

    while (sim_pump()) {
        uint32_t start = clock_millis();
        if (sim_take_restart()) {
            seed = seed * 1664525u + 1013904223u;
            app_init(seed);
            printf("seed %u\n", (unsigned)seed);
        }
        app_set_speed(sim_fast() ? FAST_SPEED : 1);
        app_frame(fb);
        uint32_t spent = clock_millis() - start;
        if (spent < FRAME_MS) {
            sim_sleep_ms(FRAME_MS - spent);
        }
    }

    free(fb);
    return 0;
}
