#include <stdlib.h>
#include <string.h>

#include "percept_text.h"
#include "tank.h"
#include "test.h"

#define MAX_VOCAB 128

static char vocab[MAX_VOCAB][32];
static int vocab_count;

/* The tests run from tests/, so the vocabulary is one directory up. */
static void load_vocab(void)
{
    FILE *in = fopen("../model/vocab.txt", "r");
    CHECK(in != NULL);
    if (in == NULL) {
        return;
    }
    vocab_count = 0;
    while (vocab_count < MAX_VOCAB && fscanf(in, "%31s", vocab[vocab_count]) == 1) {
        vocab_count++;
    }
    fclose(in);
}

static bool in_vocab(const char *word)
{
    for (int i = 0; i < vocab_count; i++) {
        if (strcmp(vocab[i], word) == 0) {
            return true;
        }
    }
    return false;
}

static percept_t sample(void)
{
    percept_t p = {
        .species = SPECIES_CLOWNFISH,
        .hunger = 6, .energy = 7, .stress = 0,
        .bold = 8, .social = 7,
        .night = false, .startled = false,
        .current = GOAL_WANDER,
        .food = DIST_NEAR, .friend_ = DIST_MID, .finger = DIST_NONE, .cover = DIST_FAR,
    };
    return p;
}

static void test_the_documented_example(void)
{
    percept_t p = sample();
    char line[PERCEPT_TEXT_MAX];
    int n = percept_encode(&p, line, sizeof(line));
    const char *expected = "species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day "
                           "startled no doing wander food near friend mid finger none cover far";
    CHECK(strcmp(line, expected) == 0);
    CHECK(n == (int)strlen(expected));

    p.species = SPECIES_PUFFERFISH;
    p.hunger = 0;
    p.energy = 9;
    p.stress = 9;
    p.bold = 1;
    p.social = 3;
    p.night = true;
    p.startled = true;
    p.current = GOAL_SEEK_FOOD;
    p.food = DIST_NONE;
    p.friend_ = DIST_FAR;
    p.finger = DIST_NEAR;
    p.cover = DIST_MID;
    CHECK(percept_encode(&p, line, sizeof(line)) > 0);
    CHECK(strcmp(line, "species pufferfish hunger 0 energy 9 stress 9 bold 1 social 3 time night "
                       "startled yes doing seek_food food none friend far finger near cover mid") == 0);
}

static void test_every_word_is_in_the_vocabulary(void)
{
    load_vocab();
    CHECK(vocab_count == 54);
    int lines = 0, words = 0, unknown = 0, wrong_length = 0;
    char line[PERCEPT_TEXT_MAX];
    /* Every value of every field appears at least once. */
    for (int s = 0; s < SPECIES_COUNT; s++) {
        for (int g = 0; g < GOAL_COUNT; g++) {
            for (int d = 0; d < 4; d++) {
                for (int digit = 0; digit <= 9; digit++) {
                    percept_t p = sample();
                    p.species = (species_t)s;
                    p.current = (goal_t)g;
                    p.food = (dist_t)d;
                    p.friend_ = (dist_t)((d + 1) % 4);
                    p.finger = (dist_t)((d + 2) % 4);
                    p.cover = (dist_t)((d + 3) % 4);
                    p.hunger = digit;
                    p.energy = 9 - digit;
                    p.stress = (digit * 3) % 10;
                    p.bold = (digit * 7) % 10;
                    p.social = (digit + 5) % 10;
                    p.night = digit % 2 == 0;
                    p.startled = digit % 3 == 0;
                    if (percept_encode(&p, line, sizeof(line)) < 0) {
                        unknown++;
                        continue;
                    }
                    lines++;
                    int count = 0;
                    for (char *word = strtok(line, " "); word != NULL; word = strtok(NULL, " ")) {
                        count++;
                        words++;
                        unknown += !in_vocab(word);
                    }
                    wrong_length += count != 26;
                }
            }
        }
    }
    CHECK(lines == SPECIES_COUNT * GOAL_COUNT * 4 * 10);
    CHECK(words == lines * 26);
    CHECK(unknown == 0);
    CHECK(wrong_length == 0);

    /* And the answer's words are there too. */
    for (int g = 0; g < GOAL_COUNT; g++) {
        CHECK(in_vocab(goal_name((goal_t)g)));
    }
    CHECK(in_vocab("calm") && in_vocab("eager") && in_vocab("wary") && in_vocab("playful") &&
          in_vocab("sleepy") && in_vocab("sulky"));
    CHECK(in_vocab("->") && in_vocab("<s>") && in_vocab("</s>"));
}

static void test_bad_percepts_are_refused(void)
{
    char line[PERCEPT_TEXT_MAX];
    percept_t p;

    p = sample();
    p.hunger = 10;
    strcpy(line, "stale");
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    CHECK(line[0] == '\0'); /* nothing misleading left behind */

    p = sample();
    p.stress = -1;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.bold = 12;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.social = -3;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.energy = 99;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.species = SPECIES_COUNT;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.species = (species_t)-1;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.current = GOAL_COUNT;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.cover = (dist_t)7;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
    p = sample();
    p.finger = (dist_t)-1;
    CHECK(percept_encode(&p, line, sizeof(line)) == -1);
}

static void test_small_buffer(void)
{
    percept_t p = sample();
    char line[PERCEPT_TEXT_MAX];
    int full = percept_encode(&p, line, sizeof(line));
    CHECK(full > 0 && full < PERCEPT_TEXT_MAX);

    /* Exactly big enough works; one byte short is refused, not truncated. */
    char *exact = malloc((size_t)full + 1);
    CHECK(percept_encode(&p, exact, (size_t)full + 1) == full);
    free(exact);
    char *shorter = malloc((size_t)full);
    CHECK(percept_encode(&p, shorter, (size_t)full) == -1);
    CHECK(shorter[0] == '\0');
    free(shorter);
    char one[1] = {'x'};
    CHECK(percept_encode(&p, one, 1) == -1);
    CHECK(one[0] == '\0');
    CHECK(percept_encode(&p, one, 0) == -1); /* and a zero-size buffer is not touched */

    /* The longest possible line fits the documented maximum. */
    p.species = SPECIES_SURGEONFISH;
    p.night = true;
    p.startled = true;
    p.current = GOAL_INVESTIGATE;
    p.food = p.friend_ = p.finger = p.cover = DIST_NONE;
    CHECK(percept_encode(&p, line, sizeof(line)) > full);
}

static void test_tank_percepts_encode(void)
{
    /* Whatever the tank produces can always be written down. */
    tank_t t;
    tank_init(&t, 5);
    bool ok = true;
    char line[PERCEPT_TEXT_MAX];
    for (int i = 0; i < 4000 && ok; i++) {
        if (i % 97 == 0) {
            tank_set_finger(&t, (i / 97) % 2 == 0, 100 + i % 120, 60 + i % 100);
        }
        tank_tick(&t, i > 2000);
        for (int f = 0; f < TANK_FISH; f++) {
            percept_t p;
            tank_percept(&t, f, &p);
            ok = ok && p.species == t.fish[f].species && percept_encode(&p, line, sizeof(line)) > 0;
        }
    }
    CHECK(ok);
}

static void test_any_cast(void)
{
    static const species_t cast[TANK_FISH] = {SPECIES_BASS, SPECIES_SURGEONFISH, SPECIES_BASS,
                                              SPECIES_ANGELFISH, SPECIES_CATFISH};
    tank_t t, same;
    tank_init_cast(&t, 9, cast);
    tank_init_cast(&same, 9, cast);
    for (int f = 0; f < TANK_FISH; f++) {
        CHECK(t.fish[f].species == cast[f]);
        const species_info_t *info = species_info(cast[f]);
        CHECK(abs(t.fish[f].bold - info->bold) <= 1 && abs(t.fish[f].social - info->social) <= 1);
    }
    for (int i = 0; i < 1000; i++) {
        tank_tick(&t, false);
        tank_tick(&same, false);
    }
    CHECK(memcmp(&t, &same, sizeof(tank_t)) == 0);

    /* The usual cast is unchanged, and nonsense in a cast falls back to it. */
    tank_t usual, odd;
    tank_init(&usual, 3);
    static const species_t nonsense[TANK_FISH] = {SPECIES_COUNT, (species_t)-1, SPECIES_COUNT,
                                                  (species_t)99, SPECIES_COUNT};
    tank_init_cast(&odd, 3, nonsense);
    CHECK(memcmp(&usual, &odd, sizeof(tank_t)) == 0);
    tank_init_cast(&odd, 3, NULL);
    CHECK(memcmp(&usual, &odd, sizeof(tank_t)) == 0);
}

void test_percept_text(void)
{
    test_the_documented_example();
    test_every_word_is_in_the_vocabulary();
    test_bad_percepts_are_refused();
    test_small_buffer();
    test_tank_percepts_encode();
    test_any_cast();
}
