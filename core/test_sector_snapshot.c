#include "sector.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    state s;
    static const uint32_t expected_output[CORE_OUTPUT_WORDS] = {
        2880402329u,
        4233269970u,
        2324013866u,
        38567u,
        22097u,
        343u,
        346u,
        412u,
    };

    memset(&s, 0, sizeof(s));
    run_sector(&s, 42u);

    if (s.coherence_q16 != 38567u || s.entropy_q16 != 22097u ||
        s.hash64 != 18181756079169303449ULL || s.crc32 != 2324013866u ||
        s.last_entropy_milli != 343u || s.last_invariant_milli != 346u ||
        s.output_words != CORE_OUTPUT_WORDS ||
        memcmp(s.output, expected_output, sizeof(expected_output)) != 0) {
        fprintf(stderr, "snapshot mismatch\n");
        return 1;
    }

    puts("ok");
    return 0;
}
