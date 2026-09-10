#include "corpus_fractal.h"

#include <string.h>

static uint8_t majority(uint8_t a, uint8_t b, uint8_t m) {
    if (a == b && a != 2) return a;
    if (a == m && a != 2) return a;
    if (b == m && b != 2) return b;
    return 2;
}

static void enqueue(CorpusNetwork *network, uint8_t index) {
    if (!network->queued[index]) {
        network->queue[network->queue_tail] = index;
        network->queue_tail = (uint8_t)((network->queue_tail + 1) % CORPUS_QUEUE_SIZE);
        network->queued[index] = 1;
    }
}

CorpusResult corpus_solve(const uint8_t input[5], uint8_t allowed) {
    CorpusResult result;
    uint8_t candidates = 0;
    uint8_t base_open = (uint8_t)((input[0] == 2) + (input[1] == 2) + (input[2] == 2) >= 2);
    uint8_t n = (uint8_t)(base_open + (input[3] == 2) + (input[4] == 2));
    uint8_t index;

    memcpy(result.values, input, sizeof(result.values));
    result.status = CORPUS_WAIT;
    result.areas = n;
    result.support = 0;
    result.needs_base_refinement = 0;
    result.direct = 0;

    for (index = 0; index < 8; ++index) {
        uint8_t a = (uint8_t)((index >> 2) & 1);
        uint8_t b = (uint8_t)((index >> 1) & 1);
        uint8_t m = (uint8_t)(index & 1);
        uint8_t d = majority(a, b, m);
        if (!(allowed & (uint8_t)(1u << index))) continue;
        if ((input[0] != 2 && input[0] != a) ||
            (input[1] != 2 && input[1] != b) ||
            (input[2] != 2 && input[2] != m)) continue;
        if (input[3] != 2 && input[4] != 2 && ((uint8_t)(d ^ input[4]) != input[3])) continue;
        result.support |= (uint8_t)(1u << index);
        candidates++;
    }
    if (!candidates) {
        result.status = CORPUS_CONFLICT;
        return result;
    }
    if (n >= 2) return result;

    if (n == 0) {
        uint8_t d = majority(input[0], input[1], input[2]);
        if (d == 2) {
            result.needs_base_refinement = 1;
            return result;
        }
        result.status = CORPUS_CLOSED;
        result.direct = (uint8_t)(input[4] == 0);
        return result;
    }

    if (base_open) {
        uint8_t pos;
        for (pos = 0; pos < 3; ++pos) {
            uint8_t seen = 0;
            uint8_t bit;
            for (bit = 0; bit < 2; ++bit) {
                uint8_t i;
                for (i = 0; i < 8; ++i) {
                    if ((result.support & (uint8_t)(1u << i)) &&
                        (uint8_t)((i >> (2 - pos)) & 1) == bit) seen |= (uint8_t)(1u << bit);
                }
            }
            if (seen == 1 || seen == 2) {
                uint8_t value = (seen == 1) ? 0 : 1;
                if (result.values[pos] == 2) result.values[pos] = value;
            }
        }
    } else {
        uint8_t d = majority(input[0], input[1], input[2]);
        if (d != 2 && input[3] == 2) result.values[3] = (uint8_t)(d ^ input[4]);
        if (d != 2 && input[4] == 2) result.values[4] = (uint8_t)(d ^ input[3]);
    }
    result.status = CORPUS_WAIT;
    for (index = 0; index < 5; ++index) {
        if (result.values[index] != input[index]) result.status = CORPUS_CHANGED;
    }
    return result;
}

void corpus_network_init(CorpusNetwork *network) {
    memset(network, 0, sizeof(*network));
    for (uint8_t i = 0; i < CORPUS_CELLS; ++i) network->cells[i].value = 2;
}

int corpus_network_add(CorpusNetwork *network, const uint8_t cells[5], uint8_t allowed) {
    uint8_t index;
    if (network->gate_count >= CORPUS_MAX_GATES) return -1;
    if (allowed > 255) return -1;
    index = network->gate_count++;
    memcpy(network->gates[index].cells, cells, 5);
    network->gates[index].allowed = allowed;
    enqueue(network, index);
    return index;
}

int corpus_network_refine(CorpusNetwork *network, uint8_t cell, uint8_t value) {
    if (cell >= CORPUS_CELLS || value > 1) return -1;
    if (network->cells[cell].value != 2 && network->cells[cell].value != value) return -1;
    if (network->cells[cell].value == value) return 0;
    network->cells[cell].value = value;
    for (uint8_t i = 0; i < network->gate_count; ++i) {
        for (uint8_t j = 0; j < 5; ++j) {
            if (network->gates[i].cells[j] == cell) enqueue(network, i);
        }
    }
    return 0;
}

int corpus_network_step(CorpusNetwork *network, uint8_t *gate_index, CorpusResult *result) {
    uint8_t index;
    uint8_t values[5];
    if (network->failed || network->queue_head == network->queue_tail) return 0;
    index = network->queue[network->queue_head];
    network->queue_head = (uint8_t)((network->queue_head + 1) % CORPUS_QUEUE_SIZE);
    network->queued[index] = 0;
    for (uint8_t i = 0; i < 5; ++i) values[i] = network->cells[network->gates[index].cells[i]].value;
    *result = corpus_solve(values, network->gates[index].allowed);
    *gate_index = index;
    if (result->status == CORPUS_CONFLICT) network->failed = 1;
    else if (result->status == CORPUS_CHANGED) {
        for (uint8_t i = 0; i < 5; ++i) {
            uint8_t cell = network->gates[index].cells[i];
            if (result->values[i] != network->cells[cell].value)
                corpus_network_refine(network, cell, result->values[i]);
        }
    }
    return 1;
}

void corpus_fractal_init(CorpusFractalTensor *tensor) {
    corpus_network_init(&tensor->network);
    for (uint8_t i = 0; i < CORPUS_TRIPLETS; ++i)
        for (uint8_t axis = 0; axis < 3; ++axis)
            tensor->triplets[i][axis] = (uint8_t)(i * 3 + axis);
}

uint8_t corpus_fractal_seed_cell(const CorpusFractalTensor *tensor,
                                 uint8_t seed, uint8_t triplet, uint8_t axis) {
    static const uint8_t paths[4][4] = {
        {0, 1, 2, 3}, {1, 4, 5, 6}, {2, 7, 8, 9}, {3, 10, 11, 12}
    };
    (void)tensor;
    return tensor->triplets[paths[seed][triplet]][axis];
}

void corpus_window_init(CorpusWindow *window) {
    memset(window, 0, sizeof(*window));
    for (uint8_t participant = 0; participant < 3; ++participant)
        for (uint8_t position = 0; position < CORPUS_CELLS; ++position)
            window->participants[participant][position] = 2;
    for (uint8_t check = 0; check < 3; ++check)
        for (uint8_t index = 0; index < CORPUS_SEED_WIDTH; ++index) {
            window->check_refs[check][index].participant = CORPUS_OUTPUT;
            window->check_refs[check][index].position = index;
        }
    for (uint8_t check = 0; check < 3; ++check) {
        window->gate_allowed[check] = 0xff;
        for (uint8_t index = 0; index < 5; ++index) {
            window->gate_refs[check][index].participant = CORPUS_OUTPUT;
            window->gate_refs[check][index].position = index;
        }
    }
    window->dirty_checks = 0x07;
}

int corpus_window_set_check_ref(CorpusWindow *window, uint8_t check,
                                uint8_t index, uint8_t participant,
                                uint8_t position) {
    if (check >= 3 || index >= CORPUS_SEED_WIDTH || participant >= 3 ||
        position >= CORPUS_CELLS) return -1;
    window->check_refs[check][index].participant = participant;
    window->check_refs[check][index].position = position;
    return 0;
}

int corpus_window_set_gate_ref(CorpusWindow *window, uint8_t check,
                               uint8_t index, uint8_t participant,
                               uint8_t position) {
    if (check >= 3 || index >= 5 || participant >= 3 || position >= CORPUS_CELLS)
        return -1;
    window->gate_refs[check][index].participant = participant;
    window->gate_refs[check][index].position = position;
    return 0;
}

uint8_t corpus_window_check_value(const CorpusWindow *window, uint8_t check,
                                  uint8_t index) {
    CorpusCheckRef ref = window->check_refs[check][index];
    return window->participants[ref.participant][ref.position];
}

int corpus_window_mark_change(CorpusWindow *window, uint8_t participant,
                              uint8_t position) {
    if (participant >= 3 || position >= CORPUS_CELLS) return -1;
    for (uint8_t check = 0; check < 3; ++check)
        for (uint8_t index = 0; index < CORPUS_SEED_WIDTH; ++index) {
            CorpusCheckRef ref = window->check_refs[check][index];
            if (ref.participant == participant && ref.position == position)
                window->dirty_checks |= (uint8_t)(1u << check);
        }
    return 0;
}

int corpus_window_next_check(CorpusWindow *window, uint8_t *check) {
    for (uint8_t index = 0; index < 3; ++index) {
        if (window->dirty_checks & (uint8_t)(1u << index)) {
            window->dirty_checks &= (uint8_t)~(1u << index);
            *check = index;
            return 1;
        }
    }
    return 0;
}

int corpus_window_execute_check(CorpusWindow *window, uint8_t check,
                                CorpusResult *result) {
    uint8_t input[5];
    if (check >= 3) return -1;
    for (uint8_t index = 0; index < 5; ++index) {
        CorpusCheckRef ref = window->gate_refs[check][index];
        input[index] = window->participants[ref.participant][ref.position];
    }
    *result = corpus_solve(input, window->gate_allowed[check]);
    if (result->status == CORPUS_CONFLICT) return 1;
    if (result->status == CORPUS_CHANGED) {
        for (uint8_t index = 0; index < 5; ++index) {
            CorpusCheckRef ref = window->gate_refs[check][index];
            uint8_t *cell = &window->participants[ref.participant][ref.position];
            if (*cell != result->values[index]) {
                *cell = result->values[index];
                corpus_window_mark_change(window, ref.participant, ref.position);
            }
        }
    }
    return 0;
}

CorpusWindowReport corpus_window_run(CorpusWindow *window) {
    CorpusWindowReport report = {0, 0, 0, 0};
    uint8_t before[3][CORPUS_CELLS];
    memcpy(before, window->participants, sizeof(before));
    while (1) {
        uint8_t check;
        CorpusResult result;
        if (!corpus_window_next_check(window, &check)) {
            report.reached_fixed_point = 1;
            break;
        }
        report.actions++;
        if (corpus_window_execute_check(window, check, &result) != 0) {
            report.conflict = 1;
            break;
        }
    }
    for (uint8_t participant = 0; participant < 3; ++participant)
        for (uint8_t position = 0; position < CORPUS_CELLS; ++position)
            if (before[participant][position] !=
                window->participants[participant][position]) report.refinements++;
    return report;
}

uint8_t corpus_window_close_carry(CorpusWindow *window, uint8_t boundary_r,
                                  uint8_t boundary_e, CorpusWindowReport *report) {
    uint8_t *r = &window->participants[CORPUS_OUTPUT][3];
    uint8_t *e = &window->participants[CORPUS_OUTPUT][4];
    if (!window->carry_valid) return CORPUS_CARRY_PENDING;
    if (boundary_r > 1 || boundary_e != 0) return CORPUS_CARRY_PRUNED;
    if ((*r != 2 && *r != boundary_r) || (*e != 2 && *e != 0))
        return CORPUS_CARRY_PRUNED;
    if (*r == 2) {
        *r = boundary_r;
        corpus_window_mark_change(window, CORPUS_OUTPUT, 3);
    }
    if (*e == 2) {
        *e = 0;
        corpus_window_mark_change(window, CORPUS_OUTPUT, 4);
    }
    *report = corpus_window_run(window);
    if (report->conflict || *r != boundary_r || *e != 0)
        return CORPUS_CARRY_PRUNED;
    window->carry_valid = 0;
    return CORPUS_CARRY_CLOSED;
}

uint8_t corpus_window_validate_checks(const CorpusWindow *window) {
    for (uint8_t index = 0; index < CORPUS_SEED_WIDTH; ++index) {
        if (corpus_window_check_value(window, CORPUS_CHECK_DS, index) != 1) return 0;
        if (corpus_window_check_value(window, CORPUS_CHECK_DE, index) != 0) return 0;
    }
    return 1;
}

uint8_t corpus_window_classify(CorpusWindow *window, uint8_t top_r, uint8_t top_e) {
    if (top_r > 2 || top_e > 2) return CORPUS_WINDOW_PENDING;
    if (!corpus_window_validate_checks(window)) return CORPUS_WINDOW_PENDING;
    if (top_r == 2) {
        window->carry = top_e;
        window->carry_valid = 1;
        return CORPUS_WINDOW_CARRY;
    }
    window->carry_valid = 0;
    if (top_e == 1) return CORPUS_WINDOW_INCONGRUENT;
    if (top_e == 2) return CORPUS_WINDOW_PENDING;
    return CORPUS_WINDOW_COHERENT;
}

void corpus_branch_init(CorpusBranch *branch) {
    memset(branch, 0, sizeof(*branch));
    corpus_window_init(&branch->window);
    branch->active = 1;
    branch->priority = 0;
}

int corpus_branch_fork(const CorpusBranch *source, CorpusBranch *target,
                       uint8_t parent, uint8_t depth) {
    if (!source || !target || !source->active || source->pruned ||
        depth == 0xff) return -1;
    memcpy(target, source, sizeof(*target));
    target->active = 1;
    target->pruned = 0;
    target->parent = parent;
    target->depth = depth;
    target->priority = source->priority;
    target->complete = 0;
    return 0;
}

void corpus_branch_set_priority(CorpusBranch *branch, uint16_t priority) {
    if (branch) branch->priority = priority;
}

int corpus_branch_refine(CorpusBranch *branch, uint8_t participant,
                         uint8_t position, uint8_t value) {
    uint8_t *cell;
    if (!branch || !branch->active || branch->pruned || participant >= 3 ||
        position >= CORPUS_CELLS || value > 1) return -1;
    cell = &branch->window.participants[participant][position];
    if (*cell != 2 && *cell != value) {
        branch->active = 0;
        branch->pruned = 1;
        return 1;
    }
    if (*cell != value) {
        *cell = value;
        corpus_window_mark_change(&branch->window, participant, position);
    }
    return 0;
}

CorpusWindowReport corpus_branch_run(CorpusBranch *branch) {
    CorpusWindowReport report = {0, 0, 0, 0};
    if (!branch || !branch->active || branch->pruned) {
        report.conflict = 1;
        return report;
    }
    report = corpus_window_run(&branch->window);
    if (report.conflict) {
        branch->active = 0;
        branch->pruned = 1;
    } else {
        branch->complete = (uint8_t)(report.reached_fixed_point &&
                                     !branch->window.carry_valid &&
                                     corpus_window_validate_checks(&branch->window));
    }
    return report;
}

uint8_t corpus_branch_is_viable(const CorpusBranch *branch) {
    return (uint8_t)(branch && branch->active && !branch->pruned);
}

int corpus_branch_select_best(CorpusBranch *branches, uint8_t count) {
    int selected = -1;
    for (uint8_t index = 0; index < count; ++index) {
        if (!branches[index].active || branches[index].pruned || !branches[index].complete)
            continue;
        if (selected < 0 || branches[index].priority < branches[selected].priority ||
            (branches[index].priority == branches[selected].priority &&
             branches[index].depth < branches[selected].depth)) selected = index;
    }
    for (uint8_t index = 0; index < count; ++index)
        if ((int)index != selected && branches[index].active) branches[index].active = 0;
    return selected;
}

void corpus_branch_queue_init(CorpusBranchQueue *queue) {
    memset(queue, 0, sizeof(*queue));
    queue->selected = 0xff;
}

int corpus_branch_queue_add(CorpusBranchQueue *queue, const CorpusBranch *branch) {
    if (!queue || !branch || queue->count >= CORPUS_MAX_BRANCHES) return -1;
    memcpy(&queue->branches[queue->count], branch, sizeof(*branch));
    queue->count++;
    return (int)(queue->count - 1);
}

int corpus_branch_queue_extend(CorpusBranchQueue *queue, uint8_t boundary_r,
                               uint8_t boundary_e) {
    if (!queue || boundary_r > 1 || boundary_e != 0) return -1;
    for (uint8_t index = 0; index < queue->count; ++index) {
        CorpusBranch *branch = &queue->branches[index];
        CorpusWindowReport report;
        if (!corpus_branch_is_viable(branch)) continue;
        if (branch->window.carry_valid) {
            uint8_t result = corpus_window_close_carry(&branch->window,
                                                       boundary_r, boundary_e,
                                                       &report);
            if (result == CORPUS_CARRY_PRUNED) {
                branch->active = 0;
                branch->pruned = 1;
                queue->pruned_windows++;
                continue;
            }
            if (result == CORPUS_CARRY_CLOSED) {
                branch->complete = (uint8_t)(report.reached_fixed_point &&
                                             corpus_window_validate_checks(&branch->window));
                if (branch->complete) queue->completed_windows++;
            }
        } else {
            report = corpus_branch_run(branch);
            if (report.conflict) {
                queue->pruned_windows++;
                continue;
            }
            if (report.reached_fixed_point) queue->completed_windows++;
        }
    }
    return 0;
}

int corpus_branch_queue_select(CorpusBranchQueue *queue) {
    int selected;
    if (!queue) return -1;
    selected = corpus_branch_select_best(queue->branches, queue->count);
    queue->selected = selected < 0 ? 0xff : (uint8_t)selected;
    return selected;
}

void corpus_dictionary_init(CorpusDictionary *dictionary) {
    memset(dictionary, 0, sizeof(*dictionary));
}

int corpus_branch_discover_law(CorpusBranch *branch,
                               const uint8_t superior[CORPUS_CELLS],
                               const uint8_t law[CORPUS_CELLS],
                               uint16_t priority) {
    if (!branch || !corpus_branch_is_viable(branch) || branch->complete) return -1;
    memcpy(branch->provisional_superior, superior, CORPUS_CELLS);
    memcpy(branch->provisional_law, law, CORPUS_CELLS);
    branch->priority = priority;
    branch->has_provisional_law = 1;
    return 0;
}

int corpus_dictionary_promote(CorpusDictionary *dictionary, CorpusBranch *branch) {
    CorpusSpaceLaw *entry;
    if (!dictionary || !branch || !branch->complete || branch->pruned ||
        !branch->has_provisional_law || dictionary->count >= CORPUS_MAX_LAWS)
        return -1;
    entry = &dictionary->entries[dictionary->count++];
    memcpy(entry->superior, branch->provisional_superior, CORPUS_CELLS);
    memcpy(entry->law, branch->provisional_law, CORPUS_CELLS);
    entry->priority = branch->priority;
    entry->provisional = 0;
    entry->promoted = 1;
    branch->has_provisional_law = 0;
    return (int)(dictionary->count - 1);
}

int corpus_dictionary_find(const CorpusDictionary *dictionary,
                           const uint8_t superior[CORPUS_CELLS],
                           CorpusSpaceLaw *match) {
    if (!dictionary || !superior) return -1;
    for (uint8_t index = 0; index < dictionary->count; ++index) {
        if (memcmp(dictionary->entries[index].superior, superior, CORPUS_CELLS) == 0) {
            if (match) *match = dictionary->entries[index];
            return index;
        }
    }
    return -1;
}

static int compatible_space(const uint8_t stored[CORPUS_CELLS],
                            const uint8_t query[CORPUS_CELLS]) {
    for (uint8_t index = 0; index < CORPUS_CELLS; ++index)
        if (stored[index] != 2 && query[index] != 2 &&
            stored[index] != query[index]) return 0;
    return 1;
}

int corpus_dictionary_seed_branches(const CorpusDictionary *dictionary,
                                    const uint8_t superior[CORPUS_CELLS],
                                    const CorpusBranch *base,
                                    CorpusBranchQueue *queue) {
    int added = 0;
    if (!dictionary || !superior || !base || !queue || !corpus_branch_is_viable(base))
        return -1;
    for (uint8_t law_index = 0; law_index < dictionary->count; ++law_index) {
        const CorpusSpaceLaw *law = &dictionary->entries[law_index];
        CorpusBranch candidate;
        if (!compatible_space(law->superior, superior)) continue;
        if (corpus_branch_fork(base, &candidate, 0, (uint8_t)(base->depth + 1)) != 0)
            return -1;
        for (uint8_t position = 0; position < CORPUS_CELLS; ++position) {
            uint8_t value = law->law[position];
            if (value != 2 && corpus_branch_refine(&candidate, CORPUS_KNOWLEDGE,
                                                   position, value) != 0) {
                candidate.pruned = 1;
                candidate.active = 0;
                break;
            }
        }
        if (!candidate.pruned) {
            candidate.priority = law->priority;
            candidate.has_provisional_law = 1;
            memcpy(candidate.provisional_superior, law->superior, CORPUS_CELLS);
            memcpy(candidate.provisional_law, law->law, CORPUS_CELLS);
            if (corpus_branch_queue_add(queue, &candidate) < 0) return -1;
            added++;
        }
    }
    return added;
}

CorpusOperationReport corpus_dictionary_operate(
    CorpusDictionary *dictionary, const uint8_t superior[CORPUS_CELLS],
    const CorpusBranch *base, uint8_t boundary_r, uint8_t boundary_e) {
    CorpusOperationReport report = {-1, -1, 0, 0, 0};
    CorpusBranchQueue queue;
    corpus_branch_queue_init(&queue);
    if (corpus_dictionary_seed_branches(dictionary, superior, base, &queue) < 0)
        return report;
    report.candidates = queue.count;
    if (corpus_branch_queue_extend(&queue, boundary_r, boundary_e) != 0)
        return report;
    for (uint8_t index = 0; index < queue.count; ++index)
        if (queue.branches[index].pruned) report.pruned++;
        else if (queue.branches[index].complete) report.completed++;
    report.selected_branch = corpus_branch_queue_select(&queue);
    if (report.selected_branch >= 0) {
        CorpusBranch *selected = &queue.branches[report.selected_branch];
        report.promoted_law = corpus_dictionary_promote(dictionary, selected);
    }
    return report;
}
