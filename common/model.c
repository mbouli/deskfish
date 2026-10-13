#include "model.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "percept_text.h"

#define SCHEMA_VERSION 1
#define MAX_VOCAB 255
#define MAX_WORD 23
#define MAX_LAYERS 8
#define MAX_EMBD 256
#define MAX_BLOCK 64
#define STATE_WORDS 26
#define PROMPT_TOKENS (1 + STATE_WORDS + 1) /* the start marker, the line, "->" */
#define MEMORY_SLOTS 256                     /* a power of two */
#define MEMORY_SHIFT 56                      /* 64 less the number of bits in a slot number */

/* One block's learned numbers, in the order the file holds them. */
typedef struct {
    const float *norm1_w, *norm1_b;
    const float *qkv_w, *qkv_b; /* queries, keys and values, in one grid */
    const float *out_w, *out_b;
    const float *norm2_w, *norm2_b;
    const float *up_w, *up_b;     /* the digest step widens to 4x... */
    const float *down_w, *down_b; /* ...and narrows again */
} block_t;

struct model {
    int vocab, block, layers, heads, embd;
    char words[MAX_VOCAB][MAX_WORD + 1];
    int bos, sep;
    int goal_tok[GOAL_COUNT], mood_tok[MOOD_COUNT];
    size_t count;   /* how many learned numbers */
    float *numbers; /* all of them, in one allocation */
    const float *tok_emb, *pos_emb, *norm_w, *norm_b, *to_vocab;
    block_t blocks[MAX_LAYERS];
    /* Working space for one forward pass. */
    float *x, *qkv, *normed, *gathered, *hidden, *delta, *weights;
    void (*release)(void *p); /* how to give this model's memory back */

    /* Recent situations and the answers given, each situation packed into
     * one number. A key of 0 is an empty slot. */
    bool remembering;
    unsigned recalled;
    struct {
        uint64_t key;
        uint8_t goal, mood;
    } memory[MEMORY_SLOTS];
};

/* Where the next model loaded keeps its numbers. */
static void *(*alloc_fn)(size_t size);
static void (*release_fn)(void *p);

void model_use_memory(void *(*alloc)(size_t size), void (*release)(void *p))
{
    alloc_fn = alloc != NULL && release != NULL ? alloc : NULL;
    release_fn = alloc_fn != NULL ? release : NULL;
}

/* ---- Reading the file -------------------------------------------------- */

typedef struct {
    const uint8_t *data;
    size_t size, at;
    bool ok;
} reader_t;

static const uint8_t *take(reader_t *r, size_t count)
{
    if (!r->ok || r->size - r->at < count) {
        r->ok = false;
        return NULL;
    }
    const uint8_t *p = r->data + r->at;
    r->at += count;
    return p;
}

static uint32_t take_u32(reader_t *r)
{
    const uint8_t *p = take(r, 4);
    if (p == NULL) {
        return 0;
    }
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Unpack one grid of `expect` numbers to floats at *next, and move *next on. */
static const float *take_tensor(reader_t *r, int bits, size_t expect, float **next)
{
    uint32_t count = take_u32(r);
    uint32_t scale_bits = take_u32(r);
    float scale;
    memcpy(&scale, &scale_bits, sizeof(scale));
    if (!r->ok || count != expect || !isfinite(scale)) {
        r->ok = false;
        return NULL;
    }
    const uint8_t *p = take(r, bits == 8 ? expect : (expect + 1) / 2);
    if (p == NULL) {
        return NULL;
    }
    float *out = *next;
    for (size_t i = 0; i < expect; i++) {
        int v;
        if (bits == 8) {
            v = (int8_t)p[i];
        } else {
            v = (int)((p[i / 2] >> (i % 2 ? 4 : 0)) & 0x0F) - 8;
        }
        out[i] = (float)v * scale;
    }
    *next += expect;
    return out;
}

static int find_word(const model_t *m, const char *word, size_t len)
{
    for (int i = 0; i < m->vocab; i++) {
        if (strlen(m->words[i]) == len && memcmp(m->words[i], word, len) == 0) {
            return i;
        }
    }
    return -1;
}

static int find(const model_t *m, const char *word)
{
    return find_word(m, word, strlen(word));
}

void model_free(model_t *m)
{
    if (m == NULL) {
        return;
    }
    void (*release)(void *p) = m->release;
    if (m->numbers != NULL) {
        release(m->numbers);
    }
    if (m->x != NULL) {
        release(m->x);
    }
    release(m);
}

size_t model_numbers(const model_t *m)
{
    return m == NULL ? 0 : m->count;
}

model_t *model_load(const uint8_t *data, size_t size)
{
    reader_t r = {data, size, 0, data != NULL};
    const uint8_t *magic = take(&r, 4);
    if (magic == NULL || memcmp(magic, "DFM1", 4) != 0) {
        return NULL;
    }
    uint32_t version = take_u32(&r), vocab = take_u32(&r), block = take_u32(&r), layers = take_u32(&r);
    uint32_t heads = take_u32(&r), embd = take_u32(&r), bits = take_u32(&r);
    if (!r.ok || version != SCHEMA_VERSION || (bits != 8 && bits != 4) ||
        vocab < 1 || vocab > MAX_VOCAB || block < PROMPT_TOKENS + 1 || block > MAX_BLOCK ||
        layers < 1 || layers > MAX_LAYERS || embd < 1 || embd > MAX_EMBD ||
        heads < 1 || heads > embd || embd % heads != 0) {
        return NULL;
    }

    void *(*alloc)(size_t size) = alloc_fn != NULL ? alloc_fn : malloc;
    model_t *m = alloc(sizeof(*m));
    if (m == NULL) {
        return NULL;
    }
    memset(m, 0, sizeof(*m));
    m->release = alloc_fn != NULL ? release_fn : free;
    m->remembering = true;
    m->vocab = (int)vocab;
    m->block = (int)block;
    m->layers = (int)layers;
    m->heads = (int)heads;
    m->embd = (int)embd;

    for (int i = 0; i < m->vocab; i++) {
        const uint8_t *length = take(&r, 1);
        const uint8_t *word = length == NULL ? NULL : take(&r, *length);
        if (word == NULL || *length > MAX_WORD) {
            model_free(m);
            return NULL;
        }
        memcpy(m->words[i], word, *length); /* the terminating zero is already there */
    }

    /* Every word the tank will need must be there. */
    m->bos = find(m, "<s>");
    m->sep = find(m, "->");
    bool words_ok = m->bos >= 0 && m->sep >= 0;
    for (int g = 0; g < GOAL_COUNT; g++) {
        m->goal_tok[g] = find(m, goal_name((goal_t)g));
        words_ok = words_ok && m->goal_tok[g] >= 0;
    }
    for (int i = 0; i < MOOD_COUNT; i++) {
        m->mood_tok[i] = find(m, mood_name((mood_t)i));
        words_ok = words_ok && m->mood_tok[i] >= 0;
    }
    if (!words_ok) {
        model_free(m);
        return NULL;
    }

    size_t e = embd, v = vocab, b = block;
    size_t per_block = 2 * e + (3 * e * e + 3 * e) + (e * e + e) + 2 * e + (4 * e * e + 4 * e) + (4 * e * e + e);
    m->count = v * e + b * e + layers * per_block + 2 * e + v * e;
    m->numbers = alloc(m->count * sizeof(float));
    /* x, qkv (kept for every block), normed, gathered, hidden, delta, weights: one allocation. */
    size_t scratch = b * e + layers * b * 3 * e + e + e + 4 * e + e + b;
    m->x = alloc(scratch * sizeof(float));
    if (m->numbers == NULL || m->x == NULL) {
        model_free(m);
        return NULL;
    }
    m->qkv = m->x + b * e;
    m->normed = m->qkv + layers * b * 3 * e;
    m->gathered = m->normed + e;
    m->hidden = m->gathered + e;
    m->delta = m->hidden + 4 * e;
    m->weights = m->delta + e;

    float *next = m->numbers;
    int n = (int)bits;
    m->tok_emb = take_tensor(&r, n, v * e, &next);
    m->pos_emb = take_tensor(&r, n, b * e, &next);
    for (int l = 0; l < m->layers; l++) {
        block_t *k = &m->blocks[l];
        k->norm1_w = take_tensor(&r, n, e, &next);
        k->norm1_b = take_tensor(&r, n, e, &next);
        k->qkv_w = take_tensor(&r, n, 3 * e * e, &next);
        k->qkv_b = take_tensor(&r, n, 3 * e, &next);
        k->out_w = take_tensor(&r, n, e * e, &next);
        k->out_b = take_tensor(&r, n, e, &next);
        k->norm2_w = take_tensor(&r, n, e, &next);
        k->norm2_b = take_tensor(&r, n, e, &next);
        k->up_w = take_tensor(&r, n, 4 * e * e, &next);
        k->up_b = take_tensor(&r, n, 4 * e, &next);
        k->down_w = take_tensor(&r, n, 4 * e * e, &next);
        k->down_b = take_tensor(&r, n, e, &next);
    }
    m->norm_w = take_tensor(&r, n, e, &next);
    m->norm_b = take_tensor(&r, n, e, &next);
    m->to_vocab = take_tensor(&r, n, v * e, &next);
    if (!r.ok || r.at != r.size) {
        model_free(m);
        return NULL;
    }
    return m;
}

/* ---- The arithmetic ---------------------------------------------------- */

/* out = w x in + b, where w has n_out rows of n_in numbers. b may be NULL. */
static void linear(const float *w, const float *b, const float *in, int n_in, float *out, int n_out)
{
    for (int o = 0; o < n_out; o++) {
        float sum = b == NULL ? 0.0f : b[o];
        const float *row = w + (size_t)o * (size_t)n_in;
        for (int i = 0; i < n_in; i++) {
            sum += row[i] * in[i];
        }
        out[o] = sum;
    }
}

/* Shift and scale a row of numbers to mean 0 and spread 1, then apply the
 * learned scale and shift. */
static void layer_norm(const float *w, const float *b, const float *in, float *out, int n)
{
    float mean = 0.0f, var = 0.0f;
    for (int i = 0; i < n; i++) {
        mean += in[i];
    }
    mean /= (float)n;
    for (int i = 0; i < n; i++) {
        var += (in[i] - mean) * (in[i] - mean);
    }
    float inv = 1.0f / sqrtf(var / (float)n + 1e-5f);
    for (int i = 0; i < n; i++) {
        out[i] = (in[i] - mean) * inv * w[i] + b[i];
    }
}

static float gelu(float x)
{
    return 0.5f * x * (1.0f + erff(x * 0.70710678f));
}

/* Run tokens `from` up to `count` through the model, the ones before `from`
 * having been through already; scores[v] is then how strongly it expects word
 * v to come after the last one.
 *
 * Two things save most of the work. A token only ever looks back, so earlier
 * tokens are unchanged by a new one: their keys and values are kept in m->qkv
 * and only the new token is worked out. And only the last token is read at
 * the end, so the last block finishes that one alone. */
static void forward(model_t *m, const int *tokens, int from, int count, float *scores)
{
    int e = m->embd, hw = e / m->heads;
    float inv_sqrt = 1.0f / sqrtf((float)hw);

    for (int t = from; t < count; t++) {
        const float *tok = m->tok_emb + (size_t)tokens[t] * (size_t)e;
        const float *pos = m->pos_emb + (size_t)t * (size_t)e;
        for (int i = 0; i < e; i++) {
            m->x[t * e + i] = tok[i] + pos[i];
        }
    }

    for (int l = 0; l < m->layers; l++) {
        const block_t *k = &m->blocks[l];
        float *qkv = m->qkv + (size_t)l * (size_t)m->block * 3 * (size_t)e;
        /* Later blocks need every token this block finishes; nothing needs
         * what the last block makes of any token but the last. */
        int first = l == m->layers - 1 ? count - 1 : from;

        /* Gather. Every new token's query, key and value first... */
        for (int t = from; t < count; t++) {
            layer_norm(k->norm1_w, k->norm1_b, m->x + t * e, m->normed, e);
            linear(k->qkv_w, k->qkv_b, m->normed, e, qkv + t * 3 * e, 3 * e);
        }
        /* ...then each looks back over itself and everything before it. */
        for (int t = first; t < count; t++) {
            for (int h = 0; h < m->heads; h++) {
                const float *q = qkv + t * 3 * e + h * hw;
                float best = -INFINITY, total = 0.0f;
                for (int u = 0; u <= t; u++) {
                    const float *key = qkv + u * 3 * e + e + h * hw;
                    float match = 0.0f;
                    for (int i = 0; i < hw; i++) {
                        match += q[i] * key[i];
                    }
                    m->weights[u] = match * inv_sqrt;
                    if (m->weights[u] > best) {
                        best = m->weights[u];
                    }
                }
                for (int u = 0; u <= t; u++) { /* softmax: shares that add up to 1 */
                    m->weights[u] = expf(m->weights[u] - best);
                    total += m->weights[u];
                }
                float *got = m->gathered + h * hw;
                memset(got, 0, (size_t)hw * sizeof(float));
                for (int u = 0; u <= t; u++) {
                    const float *value = qkv + u * 3 * e + 2 * e + h * hw;
                    float share = m->weights[u] / total;
                    for (int i = 0; i < hw; i++) {
                        got[i] += share * value[i];
                    }
                }
            }
            linear(k->out_w, k->out_b, m->gathered, e, m->delta, e);
            for (int i = 0; i < e; i++) {
                m->x[t * e + i] += m->delta[i];
            }

            /* Digest. */
            layer_norm(k->norm2_w, k->norm2_b, m->x + t * e, m->normed, e);
            linear(k->up_w, k->up_b, m->normed, e, m->hidden, 4 * e);
            for (int i = 0; i < 4 * e; i++) {
                m->hidden[i] = gelu(m->hidden[i]);
            }
            linear(k->down_w, k->down_b, m->hidden, 4 * e, m->delta, e);
            for (int i = 0; i < e; i++) {
                m->x[t * e + i] += m->delta[i];
            }
        }
    }

    layer_norm(m->norm_w, m->norm_b, m->x + (count - 1) * e, m->normed, e);
    linear(m->to_vocab, NULL, m->normed, e, scores, m->vocab);
}

static int best_of(const float *scores, const int *candidates, int count)
{
    int best = 0;
    for (int i = 1; i < count; i++) {
        if (scores[candidates[i]] > scores[candidates[best]]) {
            best = i;
        }
    }
    return best;
}

/* ---- Remembering answers ----------------------------------------------- */

/* Shift one field into the key. False if it is outside its range. */
static bool pack(uint64_t *key, int value, int count, int bits)
{
    if (value < 0 || value >= count) {
        return false;
    }
    *key = (*key << bits) | (uint64_t)value;
    return true;
}

/* A situation as one number: every field, exactly, so two situations share a
 * key only if they are the same. False if p is outside the schema. */
static bool situation_key(const percept_t *p, uint64_t *key)
{
    *key = 1; /* so that no key is 0 */
    return pack(key, (int)p->species, SPECIES_COUNT, 3) && pack(key, p->hunger, 10, 4) &&
           pack(key, p->energy, 10, 4) && pack(key, p->stress, 10, 4) && pack(key, p->bold, 10, 4) &&
           pack(key, p->social, 10, 4) && pack(key, p->night ? 1 : 0, 2, 1) &&
           pack(key, p->startled ? 1 : 0, 2, 1) && pack(key, (int)p->current, GOAL_COUNT, 3) &&
           pack(key, (int)p->food, 4, 2) && pack(key, (int)p->friend_, 4, 2) &&
           pack(key, (int)p->finger, 4, 2) && pack(key, (int)p->cover, 4, 2);
}

void model_remember(model_t *m, bool on)
{
    if (m == NULL) {
        return;
    }
    m->remembering = on;
    memset(m->memory, 0, sizeof(m->memory));
}

unsigned model_recalled(const model_t *m)
{
    return m == NULL ? 0 : m->recalled;
}

bool model_choose(model_t *m, const percept_t *p, choice_t *out)
{
    char line[PERCEPT_TEXT_MAX];
    int tokens[PROMPT_TOKENS + 1];
    float scores[MAX_VOCAB];
    int count = 0;
    if (m == NULL || p == NULL || out == NULL) {
        return false;
    }
    /* The same situation always gets the same answer, so look for it first. */
    uint64_t key = 0;
    bool keyed = m->remembering && situation_key(p, &key);
    unsigned slot = (unsigned)((key * 0x9E3779B97F4A7C15ull) >> MEMORY_SHIFT);
    if (keyed && m->memory[slot].key == key) {
        out->goal = (goal_t)m->memory[slot].goal;
        out->mood = (mood_t)m->memory[slot].mood;
        m->recalled++;
        return true;
    }
    if (percept_encode(p, line, sizeof(line)) < 0) {
        return false;
    }
    tokens[count++] = m->bos;
    for (const char *w = line; *w != '\0';) {
        const char *end = strchr(w, ' ');
        size_t len = end == NULL ? strlen(w) : (size_t)(end - w);
        int id = find_word(m, w, len);
        if (id < 0 || count >= PROMPT_TOKENS - 1) {
            return false;
        }
        tokens[count++] = id;
        w += len;
        if (*w == ' ') {
            w++;
        }
    }
    if (count != PROMPT_TOKENS - 1) {
        return false;
    }
    tokens[count++] = m->sep;

    forward(m, tokens, 0, count, scores);
    int goal = best_of(scores, m->goal_tok, GOAL_COUNT);
    tokens[count++] = m->goal_tok[goal];
    forward(m, tokens, count - 1, count, scores); /* only the new word */
    int mood = best_of(scores, m->mood_tok, MOOD_COUNT);

    if (keyed) {
        m->memory[slot].key = key;
        m->memory[slot].goal = (uint8_t)goal;
        m->memory[slot].mood = (uint8_t)mood;
    }
    out->goal = (goal_t)goal;
    out->mood = (mood_t)mood;
    return true;
}
