#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "file_helpers.h"
#include "model.h"
#include "percept_text.h"
#include "tank_helpers.h"
#include "test.h"

static void put_u32(uint8_t *at, uint32_t v)
{
    at[0] = (uint8_t)v;
    at[1] = (uint8_t)(v >> 8);
    at[2] = (uint8_t)(v >> 16);
    at[3] = (uint8_t)(v >> 24);
}

/* A copy of the good file with one 32-bit header field changed. */
static bool loads_with_field(const uint8_t *good, size_t size, int field, uint32_t value)
{
    uint8_t *bad = malloc(size);
    memcpy(bad, good, size);
    put_u32(bad + 4 + 4 * field, value);
    model_t *m = model_load(bad, size);
    bool loaded = m != NULL;
    model_free(m);
    free(bad);
    return loaded;
}

static void test_loads_the_shipped_file(const uint8_t *data, size_t size)
{
    model_t *m = model_load(data, size);
    CHECK(m != NULL);
    CHECK(model_numbers(m) == 29952);
    model_free(m);
    model_free(NULL); /* allowed */
}

static void test_refuses_bad_files(const uint8_t *data, size_t size)
{
    CHECK(model_load(NULL, 0) == NULL);
    CHECK(model_load(data, 0) == NULL);

    /* Cut short anywhere: copied out so AddressSanitizer catches a read past the end. */
    for (size_t cut = 0; cut < size; cut += (cut < 600 ? 1 : 997)) {
        uint8_t *part = malloc(cut ? cut : 1);
        memcpy(part, data, cut);
        CHECK(model_load(part, cut) == NULL);
        free(part);
    }

    uint8_t *bad = malloc(size + 1);
    memcpy(bad, data, size);
    bad[0] = 'X'; /* not DFM1 */
    CHECK(model_load(bad, size) == NULL);
    memcpy(bad, data, size);
    bad[size] = 0; /* a byte after the last tensor */
    CHECK(model_load(bad, size + 1) == NULL);
    free(bad);

    /* Header fields: 0 version, 1 vocab, 2 block, 3 layers, 4 heads, 5 width, 6 bits. */
    CHECK(!loads_with_field(data, size, 0, 2));
    CHECK(!loads_with_field(data, size, 1, 0));
    CHECK(!loads_with_field(data, size, 1, 4000000000u));
    CHECK(!loads_with_field(data, size, 2, 4));
    CHECK(!loads_with_field(data, size, 2, 4000000000u));
    CHECK(!loads_with_field(data, size, 3, 0));
    CHECK(!loads_with_field(data, size, 3, 4000000000u));
    CHECK(!loads_with_field(data, size, 4, 0));
    CHECK(!loads_with_field(data, size, 4, 3)); /* 32 does not divide by 3 */
    CHECK(!loads_with_field(data, size, 5, 0));
    CHECK(!loads_with_field(data, size, 5, 4000000000u));
    CHECK(!loads_with_field(data, size, 6, 5));

    /* The first grid of numbers claims a different length. Walk past the
     * header and the word list to find where it says so. */
    size_t at = 4 + 7 * 4;
    for (int word = 0; word < 54; word++) {
        at += 1 + data[at];
    }
    bad = malloc(size);
    memcpy(bad, data, size);
    put_u32(bad + at, 54 * 32 - 1);
    CHECK(model_load(bad, size) == NULL);
    put_u32(bad + at, 54 * 32);
    bad[at + 6] = 0x80; /* and a scale that is not a number */
    bad[at + 7] = 0x7F;
    CHECK(model_load(bad, size) == NULL);
    free(bad);
}

#define AGREEMENT_PATH "model_agreement.txt"

static int index_of(const char *word, const char *const *list, int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(word, list[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* Turn a situation line back into a percept. Only for this test. */
static bool parse_line(char *line, percept_t *p, int *goal, int *mood)
{
    static const char *const species[] = {"angelfish", "bass", "catfish", "clownfish", "goldfish", "pufferfish", "surgeonfish"};
    static const char *const goals[] = {"wander", "seek_food", "rest", "school", "investigate", "flee", "hide"};
    static const char *const dists[] = {"none", "near", "mid", "far"};
    static const char *const moods[] = {"calm", "eager", "wary", "playful", "sleepy", "sulky"};
    char *w[29];
    int n = 0;
    for (char *tok = strtok(line, " \n"); tok != NULL && n < 29; tok = strtok(NULL, " \n")) {
        w[n++] = tok;
    }
    if (n != 29) {
        return false;
    }
    memset(p, 0, sizeof(*p));
    p->species = (species_t)index_of(w[1], species, 7);
    p->hunger = atoi(w[3]);
    p->energy = atoi(w[5]);
    p->stress = atoi(w[7]);
    p->bold = atoi(w[9]);
    p->social = atoi(w[11]);
    p->night = strcmp(w[13], "night") == 0;
    p->startled = strcmp(w[15], "yes") == 0;
    p->current = (goal_t)index_of(w[17], goals, 7);
    p->food = (dist_t)index_of(w[19], dists, 4);
    p->friend_ = (dist_t)index_of(w[21], dists, 4);
    p->finger = (dist_t)index_of(w[23], dists, 4);
    p->cover = (dist_t)index_of(w[25], dists, 4);
    *goal = index_of(w[27], goals, 7);
    *mood = index_of(w[28], moods, 6);
    return *goal >= 0 && *mood >= 0;
}

static void test_agrees_with_python(model_t *m)
{
    FILE *f = fopen(AGREEMENT_PATH, "r");
    CHECK(f != NULL);
    if (f == NULL) {
        return;
    }
    char line[400], copy[400], back[PERCEPT_TEXT_MAX];
    int lines = 0, same = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        percept_t p;
        int goal, mood;
        strcpy(copy, line);
        CHECK(parse_line(copy, &p, &goal, &mood));
        /* The percept we rebuilt must write the same line Python read. */
        CHECK(percept_encode(&p, back, sizeof(back)) > 0);
        CHECK(strncmp(back, line, strlen(back)) == 0);
        choice_t c = {GOAL_COUNT, MOOD_COUNT};
        CHECK(model_choose(m, &p, &c));
        lines++;
        if ((int)c.goal == goal && (int)c.mood == mood) {
            same++;
        } else {
            printf("  differs: %s    C says %s %d\n", line, goal_name(c.goal), (int)c.mood);
        }
    }
    fclose(f);
    CHECK(lines == 500);
    CHECK(same == lines);
}

static void test_choose_is_safe(model_t *m)
{
    percept_t p;
    memset(&p, 0, sizeof(p));
    p.hunger = 3;
    p.energy = 6;
    p.bold = 5;
    p.social = 5;
    choice_t a = {GOAL_COUNT, MOOD_COUNT}, b = {GOAL_COUNT, MOOD_COUNT};
    CHECK(model_choose(m, &p, &a));
    CHECK(model_choose(m, &p, &b));
    CHECK((int)a.goal >= 0 && a.goal < GOAL_COUNT && (int)a.mood >= 0 && a.mood < MOOD_COUNT);
    CHECK(a.goal == b.goal && a.mood == b.mood); /* the same question, the same answer */

    choice_t untouched = {GOAL_HIDE, MOOD_SULKY};
    p.hunger = 12; /* outside the schema */
    CHECK(!model_choose(m, &p, &untouched));
    CHECK(untouched.goal == GOAL_HIDE && untouched.mood == MOOD_SULKY);
    CHECK(!model_choose(NULL, &p, &untouched));
}

static model_t *shared;
static choice_t ask_shared(const percept_t *p)
{
    choice_t c = {GOAL_COUNT, MOOD_COUNT};
    model_choose(shared, p, &c);
    return c;
}

static void test_same_seed_same_tank(model_t *m)
{
    tank_t a, b;
    script_t sa = {7, false, 0, 0}, sb = {7, false, 0, 0};
    shared = m;
    tank_init(&a, 21);
    tank_init(&b, 21);
    tank_set_brain(&a, ask_shared);
    tank_set_brain(&b, ask_shared);
    bool some_mood = false;
    for (int i = 0; i < 1500; i++) {
        script_step(&sa, &a);
        script_step(&sb, &b);
        tank_tick(&a, DAY);
        tank_tick(&b, DAY);
        some_mood = some_mood || a.fish[0].mood != MOOD_CALM || a.fish[2].mood != MOOD_CALM;
    }
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    CHECK(all_ok(&a));
    CHECK(some_mood); /* the model's moods did reach the fish */
}

/* ---- Remembering answers ----------------------------------------------- */

/* A stream of situations with plenty of repeats and near-repeats. */
static void next_percept(uint32_t *state, percept_t *p)
{
    uint32_t r[4];
    for (int i = 0; i < 4; i++) {
        *state = *state * 1664525u + 1013904223u;
        r[i] = *state >> 8;
    }
    memset(p, 0, sizeof(*p));
    p->species = (species_t)(r[0] % SPECIES_COUNT);
    p->hunger = (int)(r[0] / 7 % 10);
    p->energy = (int)(r[0] / 70 % 3) + 5; /* a narrow range, so questions come round again */
    p->stress = (int)(r[1] % 3);
    p->bold = (int)(r[1] / 3 % 10);
    p->social = (int)(r[1] / 30 % 2) + 4;
    p->night = r[2] % 5 == 0;
    p->startled = r[2] / 5 % 7 == 0;
    p->current = (goal_t)(r[2] / 35 % GOAL_COUNT);
    p->food = (dist_t)(r[3] % 4);
    p->friend_ = (dist_t)(r[3] / 4 % 4);
    p->finger = (dist_t)(r[3] / 16 % 4);
    p->cover = (dist_t)(r[3] / 64 % 4);
}

static void test_remembered_answers_are_the_same(const uint8_t *data, size_t size)
{
    model_t *quick = model_load(data, size), *plain = model_load(data, size);
    CHECK(quick != NULL && plain != NULL);
    if (quick == NULL || plain == NULL) {
        model_free(quick);
        model_free(plain);
        return;
    }
    model_remember(plain, false);
    CHECK(model_recalled(quick) == 0);

    uint32_t state = 5;
    percept_t p;
    for (int round = 0; round < 2; round++) {
        state = 5; /* the second round asks every question again */
        for (int i = 0; i < 6000; i++) {
            next_percept(&state, &p);
            choice_t a = {GOAL_COUNT, MOOD_COUNT}, b = {GOAL_COUNT, MOOD_COUNT};
            CHECK(model_choose(quick, &p, &a));
            CHECK(model_choose(plain, &p, &b));
            CHECK(a.goal == b.goal && a.mood == b.mood);
            /* And again at once, as a fish whose situation has not changed does. */
            choice_t again = {GOAL_COUNT, MOOD_COUNT};
            CHECK(model_choose(quick, &p, &again));
            CHECK(again.goal == b.goal && again.mood == b.mood);
        }
    }
    CHECK(model_recalled(quick) >= 12000); /* it did answer from memory... */
    CHECK(model_recalled(plain) == 0);  /* ...and the plain one never did */

    /* A situation outside the schema is never remembered as an answer. */
    next_percept(&state, &p);
    choice_t c = {GOAL_HIDE, MOOD_SULKY};
    CHECK(model_choose(quick, &p, &c));
    p.stress = 10;
    c = (choice_t){GOAL_HIDE, MOOD_SULKY};
    CHECK(!model_choose(quick, &p, &c));
    CHECK(!model_choose(quick, &p, &c));
    CHECK(c.goal == GOAL_HIDE && c.mood == MOOD_SULKY);
    p.stress = -1;
    CHECK(!model_choose(quick, &p, &c));
    p.stress = 0;
    p.species = (species_t)SPECIES_COUNT;
    CHECK(!model_choose(quick, &p, &c));

    /* Switching memory off stops it being used; switching it on starts afresh. */
    unsigned before = model_recalled(quick);
    model_remember(quick, false);
    state = 5;
    for (int i = 0; i < 200; i++) {
        next_percept(&state, &p);
        model_choose(quick, &p, &c);
    }
    CHECK(model_recalled(quick) == before);

    model_free(quick);
    model_free(plain);
}

/* ---- Choosing where the model's numbers live --------------------------- */

static int given, taken_back, refuse_after;

static void *counting_alloc(size_t size)
{
    if (refuse_after == 0) {
        return NULL;
    }
    if (refuse_after > 0) {
        refuse_after--;
    }
    given++;
    return malloc(size);
}

static void counting_release(void *p)
{
    if (p != NULL) {
        taken_back++;
    }
    free(p);
}

static void test_memory_can_be_chosen(const uint8_t *data, size_t size)
{
    given = taken_back = 0;
    refuse_after = -1; /* never refuse */
    model_use_memory(counting_alloc, counting_release);
    model_t *m = model_load(data, size);
    CHECK(m != NULL);
    CHECK(given > 0);
    model_use_memory(NULL, NULL); /* back to the ordinary kind, while m is still alive */
    percept_t p;
    uint32_t state = 9;
    next_percept(&state, &p);
    choice_t c;
    CHECK(model_choose(m, &p, &c));
    model_free(m); /* must go back the way it came */
    CHECK(taken_back == given);

    /* Running out part-way: no model, and nothing left behind. */
    for (int allowed = 0; allowed < 4; allowed++) {
        given = taken_back = 0;
        refuse_after = allowed;
        model_use_memory(counting_alloc, counting_release);
        model_t *none = model_load(data, size);
        model_use_memory(NULL, NULL);
        if (none != NULL) { /* it needed no more than `allowed` blocks */
            model_free(none);
        }
        CHECK(taken_back == given);
    }
    refuse_after = -1;
}

void test_model(void)
{
    size_t size = 0;
    uint8_t *data = read_file(MODEL_PATH, &size);
    CHECK(data != NULL);
    if (data == NULL) {
        return;
    }
    test_loads_the_shipped_file(data, size);
    test_refuses_bad_files(data, size);
    model_t *m = model_load(data, size);
    if (m != NULL) {
        test_agrees_with_python(m);
        test_choose_is_safe(m);
        test_same_seed_same_tank(m);
    }
    model_free(m);
    test_remembered_answers_are_the_same(data, size);
    test_memory_can_be_chosen(data, size);
    free(data);
}
