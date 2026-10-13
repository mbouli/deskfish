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
#define DEBUG_EVERY_FRAMES 20

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
            "       deskfish_sim --headless [--seed S] [--ticks N] [--minutes M] [--snapshot FILE.ppm]\n"
            "keys:  F fast-forward   R restart   N real time / night / day   D print fish\n");
}

/* One line per fish: what it is doing, how it feels, and what it sees. */
static void print_fish(const tank_t *tank)
{
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &tank->fish[i];
        percept_t p;
        tank_percept(tank, i, &p);
        printf("  %-11s %-11s hunger %d energy %d stress %d | bold %d social %d | "
               "food %-4s friend %-4s finger %-4s cover %-4s | at %3.0f,%3.0f\n",
               species_info(f->species)->name, goal_name(f->goal), p.hunger, p.energy, p.stress,
               p.bold, p.social, dist_name(p.food), dist_name(p.friend_), dist_name(p.finger),
               dist_name(p.cover), (double)f->x, (double)f->y);
    }
}

/* Run with no window and print where everything ended up. */
static int run_headless(uint32_t seed, long ticks, int minutes, const char *snapshot, uint16_t *fb)
{
    tank_t tank;
    tank_init(&tank, seed);
    for (long i = 0; i < ticks; i++) {
        tank_tick(&tank, minutes);
    }
    printf("seed %u, %u ticks, %02d:%02d\n", (unsigned)seed, (unsigned)tank.tick, minutes / 60, minutes % 60);
    print_fish(&tank);
    if (snapshot != NULL) {
        render_tank(&tank, minutes, fb);
        return write_snapshot(snapshot, fb);
    }
    return 0;
}

int main(int argc, char **argv)
{
    uint32_t seed = (uint32_t)time(NULL);
    long ticks = 0;
    long minutes = 12 * 60;
    bool headless = false, minutes_given = false;
    const char *snapshot = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            ticks = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--minutes") == 0 && i + 1 < argc) {
            minutes = strtol(argv[++i], NULL, 10);
            minutes_given = true;
        } else if (strcmp(argv[i], "--snapshot") == 0 && i + 1 < argc) {
            snapshot = argv[++i];
        } else {
            usage();
            return 2;
        }
    }
    if (ticks < 0 || minutes < 0 || minutes > 1439 ||
        (!headless && (ticks != 0 || snapshot != NULL || minutes_given))) {
        usage();
        return 2;
    }

    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (headless) {
        int result = run_headless(seed, ticks, (int)minutes, snapshot, fb);
        free(fb);
        return result;
    }

    static const char *time_names[] = {"real time", "forced night", "forced day"};
    app_time_t time_mode = APP_TIME_REAL;
    bool fast = false, debug = false;
    int frame = 0;

    display_init();
    touch_init();
    app_init(seed);
    printf("seed %u\n", (unsigned)seed);

    while (sim_pump()) {
        uint32_t start = clock_millis();

        switch (sim_take_key()) {
        case 'f':
            fast = !fast;
            app_set_speed(fast ? FAST_SPEED : 1);
            break;
        case 'r':
            seed = seed * 1664525u + 1013904223u;
            app_init(seed);
            printf("seed %u\n", (unsigned)seed);
            break;
        case 'n':
            time_mode = (app_time_t)((time_mode + 1) % 3);
            app_set_time(time_mode);
            printf("time: %s\n", time_names[time_mode]);
            break;
        case 'd':
            debug = !debug;
            break;
        default:
            break;
        }

        app_frame(fb);

        if (debug && ++frame % DEBUG_EVERY_FRAMES == 0) {
            printf("tick %u%s\n", (unsigned)app_tank()->tick, app_tank()->night ? " (night)" : "");
            print_fish(app_tank());
        }

        uint32_t spent = clock_millis() - start;
        if (spent < FRAME_MS) {
            sim_sleep_ms(FRAME_MS - spent);
        }
    }

    free(fb);
    return 0;
}
