#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "clock.h"
#include "display.h"
#include "light.h"
#include "render.h"
#include "sim.h"
#include "sky.h"
#include "tank.h"
#include "touch.h"
#include "tuning.h"

#define FRAME_MS 50
#define FAST_SPEED 10
#define LAPSE_RATE 3600        /* an hour of sky per second */
#define SKIP_SECONDS (3 * 3600)
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
            "       deskfish_sim --headless [--seed S] [--ticks N] [--at YYYY-MM-DDTHH:MM] [--snapshot FILE.ppm]\n"
            "       --at is in UTC; without it, the time is now\n"
            "keys:  F fast fish   R restart   T sky time-lapse   N sky +3 hours   D print\n");
}

static const char *moon_name(float phase)
{
    static const char *names[8] = {"new", "waxing crescent", "first quarter", "waxing gibbous",
                                   "full", "waning gibbous", "last quarter", "waning crescent"};
    return names[(int)(phase * 8.0f + 0.5f) % 8];
}

static void print_sky(int64_t unix_utc, const sky_t *sky, const light_t *light)
{
    time_t when = (time_t)unix_utc;
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M", gmtime(&when));
    printf("  sky %s UTC: sun %+5.1f deg (%s, %2.0f%% through the %s), moon %s (%.2f), %d stars%s\n", stamp,
           (double)sky->sun_alt_deg, sky->sun_up ? "up" : "down", (double)(sky->progress * 100.0f),
           sky->sun_up ? "day" : "night", moon_name(sky->moon_phase), (double)sky->moon_phase,
           light->star_count, light->fish_night ? ", fish night" : "");
}

/* One line per fish: what it is doing, how it feels, and what it sees. */
static void print_fish(const tank_t *tank)
{
    for (int i = 0; i < TANK_FISH; i++) {
        const fish_t *f = &tank->fish[i];
        percept_t p;
        tank_percept(tank, i, &p);
        printf("  %-11s %-11s hunger %d energy %d stress %d | bold %d social %d | "
               "food %-4s friend %-4s finger %-4s cover %-4s | at %3.0f,%3.0f %s\n",
               species_info(f->species)->name, goal_name(f->goal), p.hunger, p.energy, p.stress,
               p.bold, p.social, dist_name(p.food), dist_name(p.friend_), dist_name(p.finger),
               dist_name(p.cover), (double)f->x, (double)f->y, f->front ? "front" : "back");
    }
}

/* Run with no window and print where everything ended up. */
static int run_headless(uint32_t seed, long ticks, int64_t unix_utc, const char *snapshot, uint16_t *fb)
{
    const place_t place = {PLACE_LAT_DEG, PLACE_LON_DEG};
    sky_t sky;
    light_t light;
    sky_compute(unix_utc, place, &sky);
    light_compute(&sky, &light);

    tank_t tank;
    tank_init(&tank, seed);
    for (long i = 0; i < ticks; i++) {
        tank_tick(&tank, light.fish_night);
    }
    printf("seed %u, %u ticks\n", (unsigned)seed, (unsigned)tank.tick);
    print_sky(unix_utc, &sky, &light);
    print_fish(&tank);
    if (snapshot != NULL) {
        render_tank(&tank, fb);
        return write_snapshot(snapshot, fb);
    }
    return 0;
}

/* "2026-06-21T18:00" to seconds since 1970. Returns false if it does not parse. */
static bool parse_at(const char *text, int64_t *out)
{
    int y, mo, d, h, mi;
    char tail;
    if (sscanf(text, "%d-%d-%dT%d:%d%c", &y, &mo, &d, &h, &mi, &tail) != 5 ||
        mo < 1 || mo > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 || mi > 59) {
        return false;
    }
    *out = sky_unix_from_utc(y, mo, d, h, mi);
    return true;
}

int main(int argc, char **argv)
{
    uint32_t seed = (uint32_t)time(NULL);
    long ticks = 0;
    int64_t at = (int64_t)time(NULL);
    bool headless = false, at_given = false;
    const char *snapshot = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            ticks = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--at") == 0 && i + 1 < argc) {
            if (!parse_at(argv[++i], &at)) {
                usage();
                return 2;
            }
            at_given = true;
        } else if (strcmp(argv[i], "--snapshot") == 0 && i + 1 < argc) {
            snapshot = argv[++i];
        } else {
            usage();
            return 2;
        }
    }
    if (ticks < 0 || (!headless && (ticks != 0 || snapshot != NULL || at_given))) {
        usage();
        return 2;
    }

    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (headless) {
        int result = run_headless(seed, ticks, at, snapshot, fb);
        free(fb);
        return result;
    }

    bool fast = false, lapse = false, debug = false;
    int frame = 0;

    display_init();
    touch_init();
    if (!app_init(seed)) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
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
        case 't':
            lapse = !lapse;
            app_set_sky_rate(lapse ? LAPSE_RATE : 1);
            printf("sky time-lapse %s\n", lapse ? "on: an hour per second" : "off");
            break;
        case 'n':
            app_skip_sky(SKIP_SECONDS);
            printf("sky jumped forward three hours\n");
            break;
        case 'd':
            debug = !debug;
            break;
        default:
            break;
        }

        app_frame(fb);

        if (debug && ++frame % DEBUG_EVERY_FRAMES == 0) {
            printf("tick %u\n", (unsigned)app_tank()->tick);
            print_sky(app_sky_time(), app_sky(), app_light());
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
