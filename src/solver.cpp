#include "solver.hpp"

#include "mem.hpp"
#include <cstring>

namespace sat {

i32 words_per_clause(Problem *problem) { return ((problem->variable_count - 1) >> 6) + 1; }

u64 get_word_mask(i32 variable_id) { return 1ul << (variable_id & 63); }

Problem init_problem(i32 variable_count, i32 clause_count) {
  assert(variable_count > 0 && clause_count > 0);

  // Add "varible x0" which is implicitly assigned a true value
  ++variable_count;

  Problem problem;
  problem.variable_count = variable_count;
  problem.clause_count   = clause_count;

  i32 clause_block_size = words_per_clause(&problem) * clause_count;

  problem.clauses = CAllocator::construct<u64>(clause_block_size);
  memset(problem.clauses, 0, u32(clause_block_size * 8));

  problem.negations = CAllocator::construct<u64>(clause_block_size);
  memset(problem.negations, 0, u32(clause_block_size * 8));

  debug("Clause Structure + Negations Bytes: %db\n", clause_block_size * 8 * 2);

  problem.unassigned      = CAllocator::construct<u64>(words_per_clause(&problem));
  problem.assigned_values = CAllocator::construct<u64>(words_per_clause(&problem));
  for (i32 i = 0; i < words_per_clause(&problem) - 1; ++i) {
    problem.unassigned[i] = (u64)-1;
  }
  if ((variable_count & 63) == 0) {
    problem.unassigned[words_per_clause(&problem) - 1] = (u64)-1;
  } else {
    problem.unassigned[words_per_clause(&problem) - 1] = get_word_mask(variable_count) - 1ul;
  }

  // Set variable 0 to be assigned
  problem.unassigned[0] &= ~(u64)1;
  problem.assigned_values[0] |= 1;

  debug("Unassigned: ");
  for (i32 i = 0; i < words_per_clause(&problem); ++i) {
    debug(" %016lx", problem.unassigned[i]);
  }
  debug("\n");

  problem.decision_stack_size       = 0;
  problem.decision_stack            = CAllocator::construct<i32>(variable_count);
  problem.previous_unassigned_stack = CAllocator::construct<u64 *>(variable_count);

  problem.propagation_stack_size = 0;
  problem.propagation_stack      = CAllocator::construct<i32>(variable_count);

  problem.variable_to_clause = CAllocator::construct<SimpleWatchlistNode *>(variable_count);
  for (i32 i = 0; i < variable_count; ++i) {
    problem.variable_to_clause[i] = nullptr;
  }

  return problem;
}

bool is_negated(Problem *problem, i32 clause_id, i32 variable_id) {
  assert(clause_id >= 0 && clause_id < problem->clause_count);
  assert(variable_id > 0 && variable_id < problem->variable_count);
  return problem->negations[clause_id * words_per_clause(problem) + (variable_id >> 6)] & get_word_mask(variable_id);
}

bool is_assigned(Problem *problem, i32 variable_id) {
  assert(variable_id > 0 && variable_id < problem->variable_count);
  return !(problem->unassigned[variable_id >> 6] & get_word_mask(variable_id));
}

void push_propagation(Problem *problem, i32 variable_id, bool value) {
  assert(problem->propagation_stack_size < problem->variable_count);

  for (i32 i = 0; i < problem->propagation_stack_size; ++i) {
    assert((problem->propagation_stack[i] & 0x7FFFFFFF) != variable_id);
  }

  if (value) variable_id |= (1ll << 31);
  problem->propagation_stack[problem->propagation_stack_size++] = variable_id;
}

i32 top_propagation_stack(Problem *problem) {
  assert(problem->propagation_stack_size > 0);
  return problem->propagation_stack[problem->propagation_stack_size - 1];
}

void add_variable(Problem *problem, i32 clause_id, i32 variable_id, bool negate) {
  assert(clause_id >= 0 && clause_id < problem->clause_count);
  assert(variable_id > 0 && variable_id < problem->variable_count);

  i32 index = (clause_id * words_per_clause(problem)) + (variable_id >> 6);

  if (problem->clauses[index] & get_word_mask(variable_id)) {
    // Case where we encountered something like "(~x v x)"

    // problem->clauses[index] |= ~get_variable_mask(variable_id);
    assert(!"TODO: perform trick with setting variable 0 when we get something like (~x v x)");
    return;
  }

  problem->clauses[index] |= get_word_mask(variable_id);

  if (negate) problem->negations[index] |= get_word_mask(variable_id);
}

void set_variable(Problem *problem, i32 variable_id, bool value) {
  assert(variable_id > 0 && variable_id < problem->variable_count);
  i32 index = variable_id >> 6;

  // Ensure that variable is unknown before being set
  assert(problem->unassigned[index] & get_word_mask(variable_id));

  problem->unassigned[index] &= ~get_word_mask(variable_id);
  if (value) {
    problem->assigned_values[index] |= get_word_mask(variable_id);
  } else {
    problem->assigned_values[index] &= ~get_word_mask(variable_id);
  }

  push_propagation(problem, variable_id, value);
}

void push_new_decision(Problem *problem, i32 variable_id, bool value) {
  assert(problem->decision_stack_size < problem->variable_count);
  if (value) variable_id |= (1ll << 31);
  u64 *previous_unassigned = CAllocator::construct<u64>(words_per_clause(problem));
  for (i32 i = 0; i < words_per_clause(problem); ++i) {
    previous_unassigned[i] = problem->unassigned[i];
  }
  problem->previous_unassigned_stack[problem->decision_stack_size] = previous_unassigned;
  problem->decision_stack[problem->decision_stack_size++]          = variable_id;
}

i32 top_decision_stack(Problem *problem) {
  assert(problem->decision_stack_size > 0);
  return problem->decision_stack[problem->decision_stack_size - 1];
}

bool decision_get_value(i32 decision) { return decision < 0; }

bool decision_is_tried_both(i32 decision) { return decision & (1 << 30); }

i32 decision_get_variable_id(i32 decision) { return decision & 0x3FFFFFFF; }

void decision_flip(i32 *decision) { *decision = *decision ^ (3 << 30); }

i32 find_variable(Problem *problem) {
  i32 variable_id = -1;

  // TODO: use correct heuristics (random, VSIDS, etc.)
  for (i32 i = 0; i < words_per_clause(problem); ++i) {
    if (problem->unassigned[i]) {
      variable_id = (i << 6) | __builtin_ctzll(problem->unassigned[i]);
      break;
    }
  }

  return variable_id;
}

enum UnitPropagateResult {
  NO_CONFLICT,
  CONFLICT,
};

UnitPropagateResult unit_propagate(Problem *problem) {
  while (problem->propagation_stack_size > 0) {
    i32 top = top_propagation_stack(problem);
    --problem->propagation_stack_size;

    bool value      = top & (1 << 31);
    i32 variable_id = top & 0x7FFFFFFF;

    // TODO: implement 2wl

    auto *current = problem->variable_to_clause[variable_id];
    while (current) {
      bool term_negated = current->clause_id >> 31;
      i32 clause_id     = current->clause_id & 0x7FFFFFFF;

      // Only propagate if value of assignment would cause a term to go to 0
      if (!value ^ term_negated) {
        bool has_unknown                = false;
        bool has_one_unknown            = false;
        i32 unknown_variable_id         = -1;
        bool unknown_variable_is_negate = false;
        for (i32 i = 0; i < words_per_clause(problem); ++i) {
          u64 assigned    = ~problem->unassigned[i];
          u64 clause_word = problem->clauses[clause_id * words_per_clause(problem) + i];
          if ((assigned & clause_word) != clause_word) {
            // Clause has an unassigned variable
            u64 unknown = (assigned & clause_word) ^ clause_word;
            assert(unknown > 0);

            has_unknown = true;

            if ((unknown & (unknown - 1)) == 0) {
              if (has_one_unknown) {
                has_one_unknown = false;
                break;
              }
              has_one_unknown            = true;
              unknown_variable_id        = (i << 6) | __builtin_ctzll(unknown);
              unknown_variable_is_negate = is_negated(problem, clause_id, unknown_variable_id);
            } else {
              has_one_unknown = false;
              break;
            }
          }
        }

        // Calculate value of the clause
        bool clause_is_true = false;
        for (i32 i = 0; i < words_per_clause(problem); ++i) {
          u64 assigned        = ~problem->unassigned[i];
          u64 clause_word     = problem->clauses[clause_id * words_per_clause(problem) + i];
          u64 clause_value    = problem->assigned_values[i];
          u64 clause_negation = problem->negations[clause_id * words_per_clause(problem) + i];

          clause_is_true |= (assigned & clause_word & (clause_value ^ clause_negation));
          if (clause_is_true) break;
        }

        if (!clause_is_true) {
          if (has_one_unknown) {
            debug("  - From clause%d: x%d = %d\n", clause_id, unknown_variable_id, !unknown_variable_is_negate);
            set_variable(problem, unknown_variable_id, !unknown_variable_is_negate);
          } else if (!has_unknown) {
            debug("  - Conflict from clause%d\n", clause_id);
            problem->propagation_stack_size = 0;
            return CONFLICT;
          }
        }
      }
      current = current->next;
    }
  }
  return NO_CONFLICT;
}

bool resolve_conflict(Problem *problem) {
  (void)problem;
  return false;
}

ProblemResult dpll_solve(Problem *problem) {
  // TODO: perform pre-processing stage

  // Check one-literal invariant
#if DEBUG
  for (i32 i = 0; i < problem->clause_count; ++i) {
    bool has_one_literal = false;
    i32 one_variable_id  = 0;

    for (i32 k = 0; k < words_per_clause(problem); ++k) {
      u64 clause_word = problem->clauses[(i * words_per_clause(problem)) + k];
      if (clause_word != 0) {
        if ((clause_word & (clause_word - 1)) == 0) {
          if (has_one_literal) {
            has_one_literal = false;
            break;
          }
          has_one_literal = true;

          one_variable_id = (k << 6) | __builtin_ctzll(clause_word);
        } else {
          has_one_literal = false;
          break;
        }
      }
    }

    if (has_one_literal) assert(is_assigned(problem, one_variable_id));
  }
#endif

  // Build variable_to_clause list for simple watchlist
  for (i32 i = 0; i < problem->clause_count; ++i) {
    for (i32 k = 0; k < words_per_clause(problem); ++k) {
      u64 clause_word = problem->clauses[(i * words_per_clause(problem)) + k];
      while (clause_word) {
        i32 offset      = __builtin_ctzll(clause_word);
        i32 variable_id = (k << 6) | offset;

        // Only add undecided variables to the watchlist because they can change
        if (!is_assigned(problem, variable_id)) {
          // Add clause to watchlist
          auto *new_node                           = CAllocator::construct<SimpleWatchlistNode>();
          new_node->clause_id                      = i | (is_negated(problem, i, variable_id) << 31);
          new_node->next                           = problem->variable_to_clause[variable_id];
          problem->variable_to_clause[variable_id] = new_node;
        }

        clause_word &= ~(1ul << offset);
      }
    }
  }

  // TODO: delete
  /*
  for (i32 i = 0; i < problem->variable_count; ++i) {
    auto *current = problem->variable_to_clause[i];
    if (current) {
      debug("x%d: ", i);
      while (current) {
        if (current->clause_id & (1 << 31)) {
          debug("~%d -> ", current->clause_id & 0x7FFFFFFF);
        } else {
          debug("%d -> ", current->clause_id);
        }
        current = current->next;
      }
      debug("\n");
    }
  }
  */

  for (;;) {
    i32 variable_id = find_variable(problem);
    if (variable_id == -1) break;

    // TODO: heuristic to decide whether to set variable to TRUE or FALSE
    bool value = true;

    set_variable(problem, variable_id, value);

    debug("Selected x%d = %d\n", variable_id, value);

    push_new_decision(problem, variable_id, value);

    while (unit_propagate(problem) == CONFLICT) {
      while (decision_is_tried_both(top_decision_stack(problem))) {
        // Backtrack by one decision level
        --problem->decision_stack_size;

        if (problem->decision_stack_size <= 0) return UNSAT;
      }

      u64 *previous_unassigned = problem->previous_unassigned_stack[problem->decision_stack_size - 1];
      for (i32 i = 0; i < words_per_clause(problem); ++i) {
        problem->unassigned[i] = previous_unassigned[i];
      }

      // Flip the decision after backtracking
      decision_flip(&problem->decision_stack[problem->decision_stack_size - 1]);

      i32 flipped_variable_id = decision_get_variable_id(top_decision_stack(problem));
      bool flipped_value      = decision_get_value(top_decision_stack(problem));

      // Unassign variable to pass assertion check in set_variable
      problem->unassigned[flipped_variable_id >> 6] |= get_word_mask(flipped_variable_id);
      set_variable(problem, flipped_variable_id, flipped_value);

      debug("Flipped x%d = %d: ", flipped_variable_id, flipped_value);

      debug("Unassigned: ");
      for (i32 i = 0; i < words_per_clause(problem); ++i) {
        debug(" %016lx", problem->unassigned[i]);
      }
      debug("\n");

      /* TODO: delete
      printf("stack: ");
      for (i32 i = 0; i < problem->decision_stack_size; ++i) {
        printf("%d ", decision_get_variable_id(problem->decision_stack[i]));
      }
      printf("\n");
      */
    }
  }

  // Verification passes
  for (i32 i = 0; i < words_per_clause(problem); ++i) {
    assert(problem->unassigned[i] == 0);
  }
  for (i32 i = 0; i < problem->clause_count; ++i) {
    bool result = false;
    for (i32 k = 0; k < words_per_clause(problem); ++k) {
      u64 clause_word = problem->clauses[(i * words_per_clause(problem)) + k];
      u64 negate_word = problem->negations[(i * words_per_clause(problem)) + k];

      while (clause_word) {
        i32 offset = __builtin_ctzll(clause_word);

        bool value      = (problem->assigned_values[k] >> offset) & 1;
        bool is_negated = (negate_word >> offset) & 1;

        if (value ^ is_negated) {
          result = true;
          break;
        }

        clause_word &= ~(1ul << offset);
      }

      if (result) break;
    }

    if (!result) {
      debug("Verification failed at clause%d\n", i);
      assert(false);
    }
  }
  debug("============================\n");
  debug("Solution verification passed\n");
  debug("============================\n");

  return SAT;
}

void print_sat_solution(Problem *problem) {
  for (i32 i = 0; i < problem->variable_count; ++i) {
    bool value = problem->assigned_values[i >> 6] & get_word_mask(i);
    printf("x%d = %d\n", i, value);
  }
}

void dump_problem(Problem *problem) {
  for (i32 i = words_per_clause(problem) - 1; i >= 0; --i) {
    for (i32 k = 0; k < 16; ++k) {
      printf("=");
    }
    if (i > 0) printf("=");
  }
  printf(" PROBLEM DUMP\n");
  for (i32 i = words_per_clause(problem) - 1; i >= 0; --i) {
    printf("%-4d        %4d ", ((i + 1) * 64) - 1, i * 64);
  }
  printf("\n");
  for (i32 i = 0; i < problem->clause_count; ++i) {
    for (i32 k = words_per_clause(problem) - 1; k >= 0; --k) {
      printf("%016lx ", problem->clauses[i * words_per_clause(problem) + k]);
    }
    printf("clause_%d\n", i);
    for (i32 k = words_per_clause(problem) - 1; k >= 0; --k) {
      printf("%016lx ", problem->negations[i * words_per_clause(problem) + k]);
    }
    printf("negate_%d\n\n", i);
  }
}

} // namespace sat
