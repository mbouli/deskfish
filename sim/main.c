#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "brain.h"
#include "clock.h"
#include "display.h"
#include "light.h"
#include "model.h"
#include "percept_text.h"
#include "render.h"
#include "scene.h"
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

#define DEFAULT_MODEL "model/out/deskfish_model.bin"

/* A whole file in memory, for the caller to free. NULL if it cannot be read. */
static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    uint8_t *data = NULL;
    long length = -1;
    if (fseek(f, 0, SEEK_END) == 0 && (length = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0) {
        data = malloc((size_t)length + 1);
        if (data != NULL && fread(data, 1, (size_t)length, f) != (size_t)length) {
            free(data);
            data = NULL;
        }
    }
    fclose(f);
    *size = data != NULL ? (size_t)length : 0;
    return data;
}

/* The model for a headless run, which has a tank of its own rather than the app's. */
static model_t *headless_model;

static choice_t headless_brain(const percept_t *p)
{
    choice_t c = {GOAL_COUNT, MOOD_COUNT};
    model_choose(headless_model, p, &c);
    return c;
}

static void usage(void)
{
    fprintf(stderr,
            "usage: deskfish_sim [--seed S] [--model FILE | --rules]\n"
            "       deskfish_sim --headless [--seed S] [--ticks N] [--at YYYY-MM-DDTHH:MM] [--snapshot FILE.ppm]\n"
            "                    [--model FILE | --rules]\n"
            "       --at is in UTC; without it, the time is now\n"
            "       fish listen to the model in " DEFAULT_MODEL " unless told otherwise;\n"
            "       --rules has them follow the hand-written rules\n"
            "       deskfish_sim --headless --traces FILE [--runs N] [--ticks N] [--seed S]\n"
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
        printf("  %-11s %-11s %-7s hunger %d energy %d stress %d | bold %d social %d | "
               "food %-4s friend %-4s finger %-4s cover %-4s | at %3.0f,%3.0f %s\n",
               species_info(f->species)->name, goal_name(f->goal), mood_name(f->mood), p.hunger, p.energy, p.stress,
               p.bold, p.social, dist_name(p.food), dist_name(p.friend_), dist_name(p.finger),
               dist_name(p.cover), (double)f->x, (double)f->y, f->front ? "front" : "back");
    }
}

/* Run with no window and print where everything ended up. */
static int run_headless(uint32_t seed, long ticks, int64_t unix_utc, const char *snapshot, uint16_t *fb,
                        const uint8_t *model_data, size_t model_size)
{
    const place_t place = {PLACE_LAT_DEG, PLACE_LON_DEG};
    sky_t sky;
    light_t light;
    sky_compute(unix_utc, place, &sky);
    light_compute(&sky, &light);

    tank_t tank;
    tank_init(&tank, seed);
    headless_model = model_data != NULL ? model_load(model_data, model_size) : NULL;
    if (headless_model != NULL) {
        tank_set_brain(&tank, headless_brain);
    }
    for (long i = 0; i < ticks; i++) {
        tank_tick(&tank, light.fish_night);
    }
    printf("seed %u, %u ticks\n", (unsigned)seed, (unsigned)tank.tick);
    print_sky(unix_utc, &sky, &light);
    print_fish(&tank);
    if (snapshot != NULL) {
        if (!scene_init()) {
            fprintf(stderr, "out of memory\n");
            return 1;
        }
        scene_relight(&light);
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

/* ---- Traces: play tanks with a fake finger and write down every situation
 * a fish's brain is asked about, with what the rules answered. ------------ */

static FILE *trace_out;
static long trace_lines;

static choice_t tracing_brain(const percept_t *p)
{
    choice_t choice = brain_choose(p);
    char line[PERCEPT_TEXT_MAX];
    if (percept_encode(p, line, sizeof(line)) > 0) {
        fprintf(trace_out, "%s -> %s calm\n", line, goal_name(choice.goal));
        trace_lines++;
    }
    return choice;
}

static uint32_t trace_rng;

static uint32_t trace_random(uint32_t below)
{
    trace_rng = trace_rng * 1664525u + 1013904223u;
    return (trace_rng >> 8) % below;
}

static int run_traces(uint32_t seed, long runs, long ticks, const char *path)
{
    trace_out = fopen(path, "w");
    if (trace_out == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }
    trace_rng = seed != 0 ? seed : 1;
    trace_lines = 0;

    for (long run = 0; run < runs; run++) {
        /* Five of the seven species, a different five each run. */
        species_t pool[SPECIES_COUNT], cast[TANK_FISH];
        for (int s = 0; s < SPECIES_COUNT; s++) {
            pool[s] = (species_t)s;
        }
        for (int i = 0; i < TANK_FISH; i++) {
            int pick = i + (int)trace_random((uint32_t)(SPECIES_COUNT - i));
            species_t swap = pool[i];
            pool[i] = pool[pick];
            pool[pick] = swap;
            cast[i] = pool[i];
        }

        tank_t tank;
        tank_init_cast(&tank, trace_random(0x00FFFFFFu) + 1, cast);
        tank_set_brain(&tank, tracing_brain);
        /* Start every fish somewhere different in its needs, so the run does
         * not spend its whole length warming up. */
        for (int f = 0; f < TANK_FISH; f++) {
            tank.fish[f].hunger = (float)trace_random(10);
            tank.fish[f].energy = (float)trace_random(10);
            tank.fish[f].stress = (float)trace_random(10);
        }

        bool down = false;
        int held = 0, x = 0, y = 0;
        for (long tick = 0; tick < ticks; tick++) {
            if (down && --held <= 0) {
                down = false;
            } else if (!down && trace_random(60) == 0) {
                down = true;
                held = trace_random(10) < 6 ? 2 : 20 + (int)trace_random(140); /* a tap, or a hold */
                if (trace_random(2) == 0) { /* aimed at a fish, give or take */
                    const fish_t *target = &tank.fish[trace_random(TANK_FISH)];
                    x = (int)target->x - 30 + (int)trace_random(61);
                    y = (int)target->y - 30 + (int)trace_random(61);
                } else {
                    x = (int)trace_random(DISPLAY_W);
                    y = (int)trace_random(DISPLAY_H);
                }
            } else if (down && trace_random(3) == 0) {
                x += (int)trace_random(7) - 3; /* a held finger wanders a little */
                y += (int)trace_random(7) - 3;
            }
            x = x < 0 ? 0 : x >= DISPLAY_W ? DISPLAY_W - 1 : x;
            y = y < 0 ? 0 : y >= DISPLAY_H ? DISPLAY_H - 1 : y;
            tank_set_finger(&tank, down, x, y);

            bool night = ((tick / 1500) + run) % 3 == 0; /* a third of the time */
            tank_tick(&tank, night);
        }
    }
    fclose(trace_out);
    printf("wrote %ld lines from %ld runs of %ld ticks to %s\n", trace_lines, runs, ticks, path);
    return 0;
}

int main(int argc, char **argv)
{
    uint32_t seed = (uint32_t)time(NULL);
    long ticks = 0;
    int64_t at = (int64_t)time(NULL);
    bool headless = false, at_given = false;
    const char *snapshot = NULL;
    long runs = 20;
    bool runs_given = false;
    const char *traces = NULL;
    const char *model_path = DEFAULT_MODEL;
    bool rules = false, model_given = false;

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
        } else if (strcmp(argv[i], "--traces") == 0 && i + 1 < argc) {
            traces = argv[++i];
        } else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) {
            model_path = argv[++i];
            model_given = true;
        } else if (strcmp(argv[i], "--rules") == 0) {
            rules = true;
        } else if (strcmp(argv[i], "--runs") == 0 && i + 1 < argc) {
            runs = strtol(argv[++i], NULL, 10);
            runs_given = true;
        } else {
            usage();
            return 2;
        }
    }
    if (ticks < 0 || runs < 1 || (rules && model_given) || (traces != NULL && (rules || model_given)) ||
        (!headless && (ticks != 0 || snapshot != NULL || at_given || traces != NULL || runs_given))) {
        usage();
        return 2;
    }

    uint16_t *fb = malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    if (fb == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (headless && traces != NULL) {
        int result = run_traces(seed, runs, ticks > 0 ? ticks : 6000, traces);
        free(fb);
        return result;
    }
    /* Fish listen to the model unless told to follow the rules. Traces are
     * always the rules' answers, so they never get here. */
    uint8_t *model_data = NULL;
    size_t model_size = 0;
    if (!rules) {
        model_data = read_file(model_path, &model_size);
        model_t *check = model_data != NULL ? model_load(model_data, model_size) : NULL;
        if (check != NULL) {
            printf("model: %s (%u numbers)\n", model_path, (unsigned)model_numbers(check));
            model_free(check);
        } else {
            printf("model: cannot use %s; fish are following the rules\n", model_path);
            free(model_data);
            model_data = NULL;
        }
    } else {
        printf("model: not used; fish are following the rules\n");
    }

    if (headless) {
        int result = run_headless(seed, ticks, at, snapshot, fb, model_data, model_size);
        free(model_data);
        free(fb);
        return result;
    }

    bool fast = false, lapse = false, debug = false;
    int frame = 0;

    display_init();
    touch_init();
    if (model_data != NULL) {
        app_set_model(model_data, model_size);
        free(model_data);
    }
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
            unsigned asked = 0;
            uint32_t total_ms = 0;
            app_model_stats(&asked, &total_ms, NULL);
            if (app_using_model() && asked > 0) {
                printf("  the model has decided %u times, %.2f ms each on average\n", asked, (double)total_ms / asked);
            }
        }

        uint32_t spent = clock_millis() - start;
        if (spent < FRAME_MS) {
            sim_sleep_ms(FRAME_MS - spent);
        }
    }

    free(fb);
    return 0;
}
