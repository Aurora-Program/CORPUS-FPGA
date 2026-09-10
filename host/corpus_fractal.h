#ifndef CORPUS_FRACTAL_H
#define CORPUS_FRACTAL_H

#include <stdint.h>

#define CORPUS_TRIPLETS 13
#define CORPUS_CELLS (CORPUS_TRIPLETS * 3)
#define CORPUS_MAX_GATES 64
#define CORPUS_QUEUE_SIZE CORPUS_MAX_GATES
#define CORPUS_SEED_WIDTH 3
#define CORPUS_MAX_BRANCHES 16
#define CORPUS_MAX_WINDOWS 8
#define CORPUS_MAX_LAWS 16

enum corpus_status {
    CORPUS_WAIT = 0,
    CORPUS_CHANGED = 1,
    CORPUS_CLOSED = 2,
    CORPUS_CONFLICT = 3
};

typedef struct {
    uint8_t value;
} CorpusCell;

typedef struct {
    uint8_t values[5];
    uint8_t status;
    uint8_t areas;
    uint8_t support;
    uint8_t needs_base_refinement;
    uint8_t direct;
} CorpusResult;

typedef struct {
    uint8_t cells[5];
    uint8_t allowed;
} CorpusGate;

typedef struct {
    CorpusCell cells[CORPUS_CELLS];
    CorpusGate gates[CORPUS_MAX_GATES];
    uint8_t gate_count;
    uint8_t queue[CORPUS_QUEUE_SIZE];
    uint8_t queue_head;
    uint8_t queue_tail;
    uint8_t queued[CORPUS_MAX_GATES];
    uint8_t failed;
} CorpusNetwork;

typedef struct {
    CorpusNetwork network;
    uint8_t triplets[CORPUS_TRIPLETS][3];
} CorpusFractalTensor;

enum corpus_participant {
    CORPUS_INPUT = 0,
    CORPUS_KNOWLEDGE = 1,
    CORPUS_OUTPUT = 2
};

enum corpus_check_seed {
    CORPUS_CHECK_DS = 0,
    CORPUS_CHECK_DE = 1,
    CORPUS_CHECK_DO = 2
};

enum corpus_window_state {
    CORPUS_WINDOW_PENDING = 0,
    CORPUS_WINDOW_COHERENT = 1,
    CORPUS_WINDOW_CARRY = 2,
    CORPUS_WINDOW_INCONGRUENT = 3
};

enum corpus_carry_result {
    CORPUS_CARRY_PENDING = 0,
    CORPUS_CARRY_CLOSED = 1,
    CORPUS_CARRY_PRUNED = 2
};

typedef struct {
    uint8_t participant;
    uint8_t position;
} CorpusCheckRef;

typedef struct {
    uint8_t participants[3][CORPUS_CELLS];
    CorpusCheckRef check_refs[3][CORPUS_SEED_WIDTH];
    CorpusCheckRef gate_refs[3][5];
    uint8_t gate_allowed[3];
    uint8_t carry;
    uint8_t carry_valid;
    uint8_t dirty_checks;
} CorpusWindow;

typedef struct {
    uint16_t actions;
    uint16_t refinements;
    uint8_t conflict;
    uint8_t reached_fixed_point;
} CorpusWindowReport;

typedef struct {
    CorpusWindow window;
    uint8_t active;
    uint8_t pruned;
    uint8_t parent;
    uint8_t depth;
    uint16_t priority;
    uint8_t complete;
    uint8_t has_provisional_law;
    uint8_t provisional_superior[CORPUS_CELLS];
    uint8_t provisional_law[CORPUS_CELLS];
} CorpusBranch;

typedef struct {
    CorpusBranch branches[CORPUS_MAX_BRANCHES];
    uint8_t count;
    uint8_t selected;
    uint8_t completed_windows;
    uint8_t pruned_windows;
} CorpusBranchQueue;

typedef struct {
    uint8_t superior[CORPUS_CELLS];
    uint8_t law[CORPUS_CELLS];
    uint16_t priority;
    uint8_t provisional;
    uint8_t promoted;
} CorpusSpaceLaw;

typedef struct {
    CorpusSpaceLaw entries[CORPUS_MAX_LAWS];
    uint8_t count;
} CorpusDictionary;

typedef struct {
    int selected_branch;
    int promoted_law;
    uint8_t candidates;
    uint8_t pruned;
    uint8_t completed;
} CorpusOperationReport;

void corpus_fractal_init(CorpusFractalTensor *tensor);
uint8_t corpus_fractal_seed_cell(const CorpusFractalTensor *tensor,
                                 uint8_t seed, uint8_t triplet, uint8_t axis);
void corpus_network_init(CorpusNetwork *network);
int corpus_network_add(CorpusNetwork *network, const uint8_t cells[5], uint8_t allowed);
int corpus_network_refine(CorpusNetwork *network, uint8_t cell, uint8_t value);
int corpus_network_step(CorpusNetwork *network, uint8_t *gate_index, CorpusResult *result);
CorpusResult corpus_solve(const uint8_t values[5], uint8_t allowed);
void corpus_window_init(CorpusWindow *window);
int corpus_window_set_check_ref(CorpusWindow *window, uint8_t check,
                                uint8_t index, uint8_t participant,
                                uint8_t position);
int corpus_window_set_gate_ref(CorpusWindow *window, uint8_t check,
                               uint8_t index, uint8_t participant,
                               uint8_t position);
uint8_t corpus_window_check_value(const CorpusWindow *window, uint8_t check,
                                  uint8_t index);
int corpus_window_mark_change(CorpusWindow *window, uint8_t participant,
                              uint8_t position);
int corpus_window_next_check(CorpusWindow *window, uint8_t *check);
int corpus_window_execute_check(CorpusWindow *window, uint8_t check,
                                CorpusResult *result);
CorpusWindowReport corpus_window_run(CorpusWindow *window);
uint8_t corpus_window_close_carry(CorpusWindow *window, uint8_t boundary_r,
                                  uint8_t boundary_e, CorpusWindowReport *report);
uint8_t corpus_window_validate_checks(const CorpusWindow *window);
uint8_t corpus_window_classify(CorpusWindow *window, uint8_t top_r, uint8_t top_e);
void corpus_branch_init(CorpusBranch *branch);
int corpus_branch_fork(const CorpusBranch *source, CorpusBranch *target,
                       uint8_t parent, uint8_t depth);
void corpus_branch_set_priority(CorpusBranch *branch, uint16_t priority);
int corpus_branch_refine(CorpusBranch *branch, uint8_t participant,
                         uint8_t position, uint8_t value);
CorpusWindowReport corpus_branch_run(CorpusBranch *branch);
uint8_t corpus_branch_is_viable(const CorpusBranch *branch);
int corpus_branch_select_best(CorpusBranch *branches, uint8_t count);
void corpus_branch_queue_init(CorpusBranchQueue *queue);
int corpus_branch_queue_add(CorpusBranchQueue *queue, const CorpusBranch *branch);
int corpus_branch_queue_extend(CorpusBranchQueue *queue, uint8_t boundary_r,
                               uint8_t boundary_e);
int corpus_branch_queue_select(CorpusBranchQueue *queue);
void corpus_dictionary_init(CorpusDictionary *dictionary);
int corpus_branch_discover_law(CorpusBranch *branch, const uint8_t superior[CORPUS_CELLS],
                               const uint8_t law[CORPUS_CELLS], uint16_t priority);
int corpus_dictionary_promote(CorpusDictionary *dictionary, CorpusBranch *branch);
int corpus_dictionary_find(const CorpusDictionary *dictionary,
                           const uint8_t superior[CORPUS_CELLS],
                           CorpusSpaceLaw *match);
int corpus_dictionary_seed_branches(const CorpusDictionary *dictionary,
                                    const uint8_t superior[CORPUS_CELLS],
                                    const CorpusBranch *base,
                                    CorpusBranchQueue *queue);
CorpusOperationReport corpus_dictionary_operate(CorpusDictionary *dictionary,
                                                const uint8_t superior[CORPUS_CELLS],
                                                const CorpusBranch *base,
                                                uint8_t boundary_r,
                                                uint8_t boundary_e);

#endif
