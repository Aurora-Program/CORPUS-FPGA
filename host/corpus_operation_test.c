#include "corpus_fractal.h"

#include <stdio.h>
#include <string.h>

static int expect(int condition, const char *message) {
    if (!condition) fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

static void configure_checks(CorpusBranch *branch) {
    for (uint8_t i = 0; i < 3; ++i) {
        corpus_window_set_check_ref(&branch->window, CORPUS_CHECK_DS,
                                    i, CORPUS_INPUT, i);
        corpus_window_set_check_ref(&branch->window, CORPUS_CHECK_DE,
                                    i, CORPUS_KNOWLEDGE, i);
        corpus_window_set_gate_ref(&branch->window, CORPUS_CHECK_DS,
                                   i, CORPUS_INPUT, i);
    }
    corpus_window_set_gate_ref(&branch->window, CORPUS_CHECK_DS, 3,
                               CORPUS_OUTPUT, 3);
    corpus_window_set_gate_ref(&branch->window, CORPUS_CHECK_DS, 4,
                               CORPUS_OUTPUT, 4);
}

static void setup_base(CorpusBranch *branch) {
    corpus_branch_init(branch);
    configure_checks(branch);
    branch->window.participants[CORPUS_INPUT][0] = 1;
    branch->window.participants[CORPUS_INPUT][1] = 1;
    branch->window.participants[CORPUS_INPUT][2] = 1;
    branch->window.participants[CORPUS_OUTPUT][4] = 0;
}

int main(void) {
    CorpusDictionary dictionary;
    CorpusBranch base;
    CorpusOperationReport report;
    uint8_t superior[CORPUS_CELLS] = {0};
    uint8_t law_a[CORPUS_CELLS] = {0};
    uint8_t law_b[CORPUS_CELLS] = {0};
    uint8_t law_incompatible[CORPUS_CELLS] = {0};
    int ok = 1;

    law_b[3] = 1;
    law_incompatible[0] = 1;
    corpus_dictionary_init(&dictionary);
    setup_base(&base);
    ok &= expect(corpus_branch_discover_law(&base, superior, law_a, 5) == 0,
                 "first law discovered");
    base.complete = 1;
    ok &= expect(corpus_dictionary_promote(&dictionary, &base) == 0,
                 "first law promoted");

    setup_base(&base);
    ok &= expect(corpus_branch_discover_law(&base, superior, law_b, 2) == 0,
                 "second law discovered");
    base.complete = 1;
    ok &= expect(corpus_dictionary_promote(&dictionary, &base) == 1,
                 "second law promoted");

    memcpy(dictionary.entries[dictionary.count].superior, superior, CORPUS_CELLS);
    dictionary.entries[dictionary.count].superior[0] = 1;
    memcpy(dictionary.entries[dictionary.count].law, law_incompatible, CORPUS_CELLS);
    dictionary.entries[dictionary.count].priority = 1;
    dictionary.entries[dictionary.count].promoted = 1;
    dictionary.count++;
    setup_base(&base);
    report = corpus_dictionary_operate(&dictionary, superior, &base, 0, 0);
    ok &= expect(report.candidates == 2, "two compatible laws create two branches");
    ok &= expect(report.completed == 2 && report.selected_branch >= 0,
                 "both compatible branches complete");
    ok &= expect(report.promoted_law >= 2, "selected branch promotes a new law");

    superior[0] = 1;
    report = corpus_dictionary_operate(&dictionary, superior, &base, 0, 0);
    ok &= expect(report.candidates == 1 || report.candidates == 0,
                 "incompatible Space does not create a false candidate");

    printf("corpus_operation_c: %s laws=%u\n", ok ? "PASS" : "FAIL",
           dictionary.count);
    return ok ? 0 : 1;
}
