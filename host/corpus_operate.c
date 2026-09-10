/* CORPUS operation interface.
 * Usage: corpus_operate.exe [--demo | --query space0] [r] [e]
 */
#include "corpus_fractal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fill(uint8_t *values, uint8_t value) {
    for (uint8_t i = 0; i < CORPUS_CELLS; ++i) values[i] = value;
}

static void configure_checks(CorpusBranch *base) {
    for (uint8_t i = 0; i < 3; ++i) {
        corpus_window_set_check_ref(&base->window, CORPUS_CHECK_DS,
                                    i, CORPUS_INPUT, i);
        corpus_window_set_check_ref(&base->window, CORPUS_CHECK_DE,
                                    i, CORPUS_KNOWLEDGE, i);
        corpus_window_set_gate_ref(&base->window, CORPUS_CHECK_DS,
                                   i, CORPUS_INPUT, i);
    }
    corpus_window_set_gate_ref(&base->window, CORPUS_CHECK_DS, 3,
                               CORPUS_OUTPUT, 3);
    corpus_window_set_gate_ref(&base->window, CORPUS_CHECK_DS, 4,
                               CORPUS_OUTPUT, 4);
}

static void print_report(const CorpusOperationReport *report,
                         const CorpusDictionary *dictionary) {
    printf("candidates=%u\n", report->candidates);
    printf("completed=%u pruned=%u\n", report->completed, report->pruned);
    printf("selected_branch=%d promoted_law=%d\n",
           report->selected_branch, report->promoted_law);
    printf("dictionary_laws=%u\n", dictionary->count);
}

static int demo(void) {
    CorpusDictionary dictionary;
    CorpusBranch base;
    CorpusOperationReport report;
    uint8_t superior[CORPUS_CELLS];
    uint8_t law[CORPUS_CELLS];

    corpus_dictionary_init(&dictionary);
    corpus_branch_init(&base);
    configure_checks(&base);
    fill(superior, 0);
    fill(law, 0);
    law[0] = 0;
    base.window.participants[CORPUS_INPUT][0] = 1;
    base.window.participants[CORPUS_INPUT][1] = 1;
    base.window.participants[CORPUS_INPUT][2] = 1;
    base.window.participants[CORPUS_OUTPUT][4] = 0;
    if (corpus_branch_discover_law(&base, superior, law, 5) != 0) return 1;
    base.complete = 1;
    if (corpus_dictionary_promote(&dictionary, &base) < 0) return 1;

    corpus_branch_init(&base);
    configure_checks(&base);
    base.window.participants[CORPUS_INPUT][0] = 1;
    base.window.participants[CORPUS_INPUT][1] = 1;
    base.window.participants[CORPUS_INPUT][2] = 1;
    base.window.participants[CORPUS_OUTPUT][4] = 0;
    report = corpus_dictionary_operate(&dictionary, superior, &base, 0, 0);
    print_report(&report, &dictionary);
    return report.selected_branch >= 0 && report.promoted_law >= 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--demo") == 0)) return demo();
    fprintf(stderr, "Uso: %s [--demo]\n", argv[0]);
    return 2;
}
