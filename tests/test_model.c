#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "model.h"
#include "test.h"

#define MODEL_PATH "../model/out/deskfish_model.bin"

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)length + 1);
    if (data != NULL && fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)length;
    return data;
}

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
    free(data);
}
