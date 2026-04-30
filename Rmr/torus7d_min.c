#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define Q16_ONE 65536
#define DIMS 7
#define ATTRACTORS 42
#define ALPHA_NUM 1
#define ALPHA_DEN 4

typedef int32_t q16;

typedef struct {
    q16 v[DIMS];
} State7;

typedef struct {
    State7 s;
    q16 coherence;
    q16 entropy;
    uint32_t hash;
    uint32_t step;
} Core;

static inline q16 q16_wrap(q16 x) {
    int32_t m = x % Q16_ONE;
    if (m < 0) m += Q16_ONE;
    return (q16)m;
}

static inline q16 q16_mul(q16 a, q16 b) {
    return (q16)(((int64_t)a * (int64_t)b) >> 16);
}

static uint32_t fnv1a32(const uint8_t *p, uint32_t n) {
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t entropy_milli(const uint8_t *p, uint32_t n) {
    if (n == 0) return 0;
    uint8_t seen[256];
    memset(seen, 0, sizeof(seen));
    uint32_t uniq = 0;
    uint32_t trans = 0;
    seen[p[0]] = 1; uniq = 1;
    for (uint32_t i = 1; i < n; ++i) {
        if (!seen[p[i]]) { seen[p[i]] = 1; ++uniq; }
        if (p[i] != p[i-1]) ++trans;
    }
    uint32_t term_u = (uniq * 6000u) / 256u;
    uint32_t term_t = (n > 1) ? (trans * 2000u) / (n - 1) : 0u;
    return term_u + term_t;
}

static void toroidal_map(State7 *dst, const uint8_t *data, uint32_t n, uint32_t h, uint32_t ent_milli, uint32_t step) {
    uint32_t mix = h ^ (ent_milli * 2654435761u) ^ (step * 2246822519u);
    for (int i = 0; i < DIMS; ++i) {
        uint8_t b = n ? data[(i * 13u + step) % n] : (uint8_t)(mix >> (i * 4));
        uint32_t r = (mix >> ((i % 4) * 8)) ^ b ^ (i * 29u);
        dst->v[i] = q16_wrap((q16)((r & 0xFFFFu)));
    }
}

static uint32_t attractor42(const State7 *s) {
    uint32_t best_idx = 0;
    uint32_t best_dist = 0xFFFFFFFFu;
    for (uint32_t a = 0; a < ATTRACTORS; ++a) {
        uint32_t d = 0;
        for (int i = 0; i < DIMS; ++i) {
            uint32_t ai = (a * 1567u + (uint32_t)i * 7919u) & 0xFFFFu;
            uint32_t si = (uint32_t)s->v[i] & 0xFFFFu;
            d += (ai > si) ? (ai - si) : (si - ai);
        }
        if (d < best_dist) { best_dist = d; best_idx = a; }
    }
    return best_idx;
}

static void core_step(Core *c, const uint8_t *data, uint32_t n, q16 c_in, q16 h_in) {
    c->coherence = (q16)(((int64_t)(ALPHA_DEN - ALPHA_NUM) * c->coherence + (int64_t)ALPHA_NUM * c_in) / ALPHA_DEN);
    c->entropy   = (q16)(((int64_t)(ALPHA_DEN - ALPHA_NUM) * c->entropy   + (int64_t)ALPHA_NUM * h_in) / ALPHA_DEN);

    q16 one_minus_h = Q16_ONE - c->entropy;
    q16 phi = q16_mul(one_minus_h, c->coherence);

    toroidal_map(&c->s, data, n, c->hash, (uint32_t)h_in, c->step);

    uint32_t idx = attractor42(&c->s);
    c->hash ^= idx;

    const uint8_t *sp = (const uint8_t*)c->s.v;
    c->hash = fnv1a32(sp, sizeof(c->s.v)) ^ (uint32_t)phi ^ (idx << 24) ^ c->hash;
    c->step++;
}

int main(int argc, char **argv) {
    const uint8_t *payload = (const uint8_t*)(argc > 1 ? argv[1] : "RafCoder");
    uint32_t n = (uint32_t)strlen((const char*)payload);

    Core c;
    memset(&c, 0, sizeof(c));
    c.coherence = Q16_ONE / 2;
    c.entropy = Q16_ONE / 8;
    c.hash = fnv1a32(payload, n);

    uint32_t em = entropy_milli(payload, n);
    q16 h_in = (q16)((em * Q16_ONE) / 8000u);
    if (h_in > Q16_ONE) h_in = Q16_ONE;

    for (int i = 0; i < 42; ++i) core_step(&c, payload, n, (q16)(Q16_ONE * 3 / 4), h_in);

    printf("TORUS7D HASH=%08x STEP=%u ATTR=%u\n", c.hash, c.step, attractor42(&c.s));
    return 0;
}
