#include "corpus_fractal.h"

#include <stdio.h>

static int expect(int condition, const char *message) {
    if (!condition) fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

int main(void) {
    CorpusFractalTensor tensor;
    CorpusNetwork network;
    CorpusResult result;
    uint8_t cells[5];
    uint8_t gate;
    int ok = 1;

    corpus_fractal_init(&tensor);
    ok &= expect(corpus_fractal_seed_cell(&tensor, 0, 1, 0) ==
                 corpus_fractal_seed_cell(&tensor, 1, 0, 0),
                 "seed views preserve shared triplet identity");
    ok &= expect(CORPUS_CELLS == 39, "fractal tensor has 39 cells");

    {
        uint8_t input[5] = {0, 0, 1, 2, 0};
        result = corpus_solve(input, 0xff);
        ok &= expect(result.status == CORPUS_CHANGED, "book example changes R");
        ok &= expect(result.values[3] == 0, "book example resolves R to zero");
    }

    corpus_network_init(&network);
    cells[0] = 0; cells[1] = 1; cells[2] = 2; cells[3] = 3; cells[4] = 4;
    ok &= expect(corpus_network_add(&network, cells, 0xff) >= 0, "gate can be queued");
    ok &= expect(corpus_network_refine(&network, 0, 0) == 0, "cell refinement succeeds");
    while (corpus_network_step(&network, &gate, &result)) {
        if (result.status == CORPUS_CONFLICT) ok = 0;
    }
    ok &= expect(!network.failed, "event network reaches a non-conflicting state");

    {
        CorpusWindow window;
        corpus_window_init(&window);
        for (uint8_t i = 0; i < CORPUS_SEED_WIDTH; ++i) {
            window.participants[CORPUS_INPUT][i] = 1;
            window.participants[CORPUS_KNOWLEDGE][i] = 0;
        }
        for (uint8_t i = 0; i < CORPUS_SEED_WIDTH; ++i) {
            corpus_window_set_check_ref(&window, CORPUS_CHECK_DS, i,
                                        CORPUS_INPUT, i);
            corpus_window_set_check_ref(&window, CORPUS_CHECK_DE, i,
                                        CORPUS_KNOWLEDGE, i);
            corpus_window_set_check_ref(&window, CORPUS_CHECK_DO, i,
                                        CORPUS_OUTPUT, i);
        }
        {
            uint8_t check;
            while (corpus_window_next_check(&window, &check)) { }
            corpus_window_mark_change(&window, CORPUS_INPUT, 1);
            ok &= expect(corpus_window_next_check(&window, &check) &&
                         check == CORPUS_CHECK_DS,
                         "input change reactivates only DS");
            ok &= expect(!corpus_window_next_check(&window, &check),
                         "unrelated checks remain idle");
            corpus_window_mark_change(&window, CORPUS_OUTPUT, 1);
            ok &= expect(corpus_window_next_check(&window, &check) &&
                         check == CORPUS_CHECK_DO,
                         "output change reactivates only DO");
        }
        ok &= expect(corpus_window_validate_checks(&window),
                     "DS=111 and DE=000 checks validate");
        ok &= expect(corpus_window_classify(&window, 2, 1) == CORPUS_WINDOW_CARRY,
                     "ambiguous superior relation produces Carry");
        ok &= expect(window.carry_valid && window.carry == 1,
                     "Carry direction is retained");
        ok &= expect(corpus_window_classify(&window, 0, 0) == CORPUS_WINDOW_COHERENT,
                     "direct coherent relation closes the window");
        ok &= expect(corpus_window_classify(&window, 0, 1) == CORPUS_WINDOW_INCONGRUENT,
                     "E=1 is incongruent for a concrete superior R");

        window.participants[CORPUS_INPUT][0] = 0;
        window.participants[CORPUS_INPUT][1] = 0;
        window.participants[CORPUS_INPUT][2] = 1;
        window.participants[CORPUS_OUTPUT][4] = 0;
        window.participants[CORPUS_OUTPUT][3] = 2;
        window.participants[CORPUS_OUTPUT][4] = 0;
        for (uint8_t i = 0; i < 3; ++i)
            corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, i,
                                       CORPUS_INPUT, i);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 3,
                                   CORPUS_OUTPUT, 3);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 4,
                                   CORPUS_OUTPUT, 4);
        corpus_window_mark_change(&window, CORPUS_INPUT, 0);
        ok &= expect(corpus_window_execute_check(&window, CORPUS_CHECK_DS,
                                                 &result) == 0,
                     "DS gate executes without conflict");
        ok &= expect(window.participants[CORPUS_OUTPUT][3] == 0,
                     "DS gate refines a shared output position");

        corpus_window_init(&window);
        window.participants[CORPUS_INPUT][0] = 0;
        window.participants[CORPUS_INPUT][1] = 0;
        window.participants[CORPUS_INPUT][2] = 1;
        window.participants[CORPUS_OUTPUT][4] = 0;
        for (uint8_t i = 0; i < 3; ++i)
            corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, i,
                                       CORPUS_INPUT, i);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 3,
                                   CORPUS_OUTPUT, 3);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 4,
                                   CORPUS_OUTPUT, 4);
        {
            CorpusWindowReport report = corpus_window_run(&window);
            ok &= expect(report.reached_fixed_point && report.actions > 0,
                         "window run reaches a fixed point");
            ok &= expect(report.refinements > 0,
                         "window run records refinements");
        }

        corpus_window_init(&window);
        window.carry_valid = 1;
        window.carry = 1;
        window.participants[CORPUS_INPUT][0] = 0;
        window.participants[CORPUS_INPUT][1] = 0;
        window.participants[CORPUS_INPUT][2] = 1;
        window.participants[CORPUS_OUTPUT][4] = 2;
        for (uint8_t i = 0; i < 3; ++i)
            corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, i,
                                       CORPUS_INPUT, i);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 3,
                                   CORPUS_OUTPUT, 3);
        corpus_window_set_gate_ref(&window, CORPUS_CHECK_DS, 4,
                                   CORPUS_OUTPUT, 4);
        {
            CorpusWindowReport report;
            ok &= expect(corpus_window_close_carry(&window, 0, 0, &report) ==
                         CORPUS_CARRY_CLOSED,
                         "Carry closes at a coherent boundary");
            ok &= expect(!window.carry_valid &&
                         window.participants[CORPUS_OUTPUT][3] == 0,
                         "Carry closure concretizes the previous relation");
        }
        window.carry_valid = 1;
        window.participants[CORPUS_OUTPUT][3] = 0;
        ok &= expect(corpus_window_close_carry(&window, 1, 0, NULL) ==
                     CORPUS_CARRY_PRUNED,
                     "incoherent boundary prunes the branch");
    }

    {
        CorpusBranch source, good, bad;
        corpus_branch_init(&source);
        source.window.participants[CORPUS_INPUT][0] = 0;
        source.window.participants[CORPUS_INPUT][1] = 0;
        source.window.participants[CORPUS_INPUT][2] = 1;
        source.window.participants[CORPUS_OUTPUT][4] = 0;
        for (uint8_t i = 0; i < 3; ++i)
            corpus_window_set_gate_ref(&source.window, CORPUS_CHECK_DS, i,
                                       CORPUS_INPUT, i);
        corpus_window_set_gate_ref(&source.window, CORPUS_CHECK_DS, 3,
                                   CORPUS_OUTPUT, 3);
        corpus_window_set_gate_ref(&source.window, CORPUS_CHECK_DS, 4,
                                   CORPUS_OUTPUT, 4);
        ok &= expect(corpus_branch_fork(&source, &good, 0, 1) == 0,
                     "good branch forks");
        ok &= expect(corpus_branch_fork(&source, &bad, 0, 1) == 0,
                     "bad branch forks independently");
        ok &= expect(corpus_branch_refine(&good, CORPUS_OUTPUT, 3, 0) == 0,
                     "compatible branch refinement succeeds");
        ok &= expect(corpus_branch_refine(&bad, CORPUS_OUTPUT, 3, 1) == 0,
                     "speculative branch accepts provisional value");
        corpus_window_mark_change(&good.window, CORPUS_INPUT, 0);
        corpus_window_mark_change(&bad.window, CORPUS_INPUT, 0);
        corpus_branch_run(&good);
        corpus_branch_run(&bad);
        ok &= expect(corpus_branch_is_viable(&good),
                     "compatible branch survives execution");
        ok &= expect(!corpus_branch_is_viable(&bad),
                     "incoherent branch is pruned");
        ok &= expect(source.window.participants[CORPUS_OUTPUT][3] == 2,
                     "source branch remains unchanged");

        {
            CorpusBranch branches[3];
            corpus_branch_init(&branches[0]);
            corpus_branch_init(&branches[1]);
            corpus_branch_init(&branches[2]);
            branches[0].complete = 1;
            branches[1].complete = 1;
            branches[2].complete = 0;
            branches[0].depth = 2;
            branches[1].depth = 1;
            corpus_branch_set_priority(&branches[0], 4);
            corpus_branch_set_priority(&branches[1], 2);
            corpus_branch_set_priority(&branches[2], 0);
            ok &= expect(corpus_branch_select_best(branches, 3) == 1,
                         "selector chooses lowest-priority complete branch");
            ok &= expect(branches[1].active && !branches[0].active &&
                         !branches[2].active,
                         "selector leaves only the chosen branch active");
        }

        {
            CorpusBranchQueue queue;
            CorpusBranch carry_branch;
            corpus_branch_init(&carry_branch);
            carry_branch.window.carry_valid = 1;
            carry_branch.window.participants[CORPUS_INPUT][0] = 1;
            carry_branch.window.participants[CORPUS_INPUT][1] = 1;
            carry_branch.window.participants[CORPUS_INPUT][2] = 1;
            carry_branch.window.participants[CORPUS_KNOWLEDGE][0] = 0;
            carry_branch.window.participants[CORPUS_KNOWLEDGE][1] = 0;
            carry_branch.window.participants[CORPUS_KNOWLEDGE][2] = 0;
            for (uint8_t i = 0; i < 3; ++i) {
                corpus_window_set_check_ref(&carry_branch.window, CORPUS_CHECK_DS,
                                            i, CORPUS_INPUT, i);
                corpus_window_set_check_ref(&carry_branch.window, CORPUS_CHECK_DE,
                                            i, CORPUS_KNOWLEDGE, i);
            }
            corpus_branch_queue_init(&queue);
            corpus_branch_queue_add(&queue, &carry_branch);
            ok &= expect(corpus_branch_queue_extend(&queue, 0, 0) == 0,
                         "branch queue extends through boundary");
            ok &= expect(queue.completed_windows == 1 && queue.pruned_windows == 0,
                         "coherent Carry window is completed");
            ok &= expect(corpus_branch_queue_select(&queue) == 0,
                         "queue selects the completed branch");
        }

        {
            CorpusBranch branch;
            CorpusDictionary dictionary;
            CorpusSpaceLaw match;
            uint8_t superior[CORPUS_CELLS] = {0};
            uint8_t law[CORPUS_CELLS] = {2};
            corpus_branch_init(&branch);
            corpus_dictionary_init(&dictionary);
            ok &= expect(corpus_branch_discover_law(&branch, superior, law, 3) == 0,
                         "branch discovers a provisional law");
            ok &= expect(corpus_dictionary_promote(&dictionary, &branch) < 0,
                         "incomplete branch cannot promote a law");
            branch.complete = 1;
            ok &= expect(corpus_dictionary_promote(&dictionary, &branch) == 0,
                         "complete branch promotes its law");
            ok &= expect(corpus_dictionary_find(&dictionary, superior, &match) == 0 &&
                         match.promoted && match.law[0] == 2,
                         "promoted law is retrievable by Space");

            {
                CorpusBranch base;
                CorpusBranchQueue candidates;
                corpus_branch_init(&base);
                corpus_branch_queue_init(&candidates);
                dictionary.entries[0].superior[0] = 0;
                dictionary.entries[0].law[0] = 1;
                dictionary.entries[0].priority = 2;
                superior[0] = 0;
                ok &= expect(corpus_dictionary_seed_branches(&dictionary, superior,
                                                             &base, &candidates) == 1,
                             "compatible law creates a Knowledge branch");
                ok &= expect(candidates.count == 1 &&
                             candidates.branches[0].window.participants[CORPUS_KNOWLEDGE][0] == 1,
                             "candidate law is loaded into K");
                superior[0] = 1;
                corpus_branch_queue_init(&candidates);
                ok &= expect(corpus_dictionary_seed_branches(&dictionary, superior,
                                                             &base, &candidates) == 0,
                             "incompatible Space creates no branch");

                dictionary.entries[0].law[0] = 0;
                superior[0] = 0;
                base.window.participants[CORPUS_INPUT][0] = 1;
                base.window.participants[CORPUS_INPUT][1] = 1;
                base.window.participants[CORPUS_INPUT][2] = 1;
                base.window.participants[CORPUS_OUTPUT][4] = 0;
                for (uint8_t i = 0; i < 3; ++i) {
                    corpus_window_set_check_ref(&base.window, CORPUS_CHECK_DS,
                                                i, CORPUS_INPUT, i);
                    corpus_window_set_check_ref(&base.window, CORPUS_CHECK_DE,
                                                i, CORPUS_KNOWLEDGE, i);
                    corpus_window_set_gate_ref(&base.window, CORPUS_CHECK_DS,
                                               i, CORPUS_INPUT, i);
                }
                corpus_window_set_gate_ref(&base.window, CORPUS_CHECK_DS, 3,
                                           CORPUS_OUTPUT, 3);
                corpus_window_set_gate_ref(&base.window, CORPUS_CHECK_DS, 4,
                                           CORPUS_OUTPUT, 4);
                {
                    CorpusOperationReport operation = corpus_dictionary_operate(
                        &dictionary, superior, &base, 0, 0);
                    ok &= expect(operation.candidates == 1,
                                 "integrated operation creates one candidate");
                    ok &= expect(operation.selected_branch >= 0 &&
                                 operation.completed == 1,
                                 "integrated operation selects complete branch");
                    ok &= expect(operation.promoted_law >= 0,
                                 "integrated operation promotes selected law");
                }
            }
        }
    }

    printf("corpus_fractal_c: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
