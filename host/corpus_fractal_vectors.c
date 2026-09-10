#include "corpus_fractal.h"

#include <stdio.h>

static unsigned encode(const uint8_t values[5]) {
    static const unsigned code[3] = {0, 3, 1};
    unsigned word = 0;
    for (unsigned i = 0; i < 5; ++i) word = (word << 2) | code[values[i]];
    return word;
}

static unsigned payload(const CorpusResult *result) {
    return (encode(result->values) << 14) |
           ((unsigned)result->status << 12) |
           ((unsigned)result->areas << 10) |
           ((unsigned)result->support << 2) |
           ((unsigned)result->needs_base_refinement << 1) |
           result->direct;
}

int main(void) {
    uint8_t values[5];
    for (values[0] = 0; values[0] <= 2; ++values[0])
    for (values[1] = 0; values[1] <= 2; ++values[1])
    for (values[2] = 0; values[2] <= 2; ++values[2])
    for (values[3] = 0; values[3] <= 2; ++values[3])
    for (values[4] = 0; values[4] <= 2; ++values[4])
    for (unsigned allowed = 0; allowed < 256; ++allowed) {
        CorpusResult result = corpus_solve(values, (uint8_t)allowed);
        printf("%03x %02x %06x\n", encode(values), allowed, payload(&result));
    }
    return 0;
}
