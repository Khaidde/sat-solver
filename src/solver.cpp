#include "solver.hpp"

#include "mem.hpp"
#include <cstring>

namespace sat {

i32 words_per_clause(Problem *problem) { return ((problem->variable_count - 1) >> 6) + 1; }

Problem init_problem(i32 variable_count, i32 clause_count) {
  assert(variable_count > 0 && clause_count > 0);

  Problem problem;
  problem.variable_count = variable_count;
  problem.clause_count   = clause_count;

  i32 clause_block_size = words_per_clause(&problem) * clause_count;

  problem.clauses = CAllocator::construct<u64>(clause_block_size);
  memset(problem.clauses, 0, u32(clause_block_size * 8));

  problem.negations = CAllocator::construct<u64>(clause_block_size);
  memset(problem.negations, 0, u32(clause_block_size * 8));

  debug("Clause Structure + Negations Bytes: %db\n", clause_block_size * 8 * 2);

  return problem;
}

void add_variable(Problem *problem, i32 clause_id, i32 variable_id, bool negate) {
  i32 index      = (clause_id * words_per_clause(problem)) + (variable_id >> 6);
  i32 clause_off = variable_id & 63;

  problem->clauses[index] |= (1 << clause_off);

  if (negate) problem->negations[index] |= (1 << clause_off);
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
