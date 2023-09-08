#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "general.hpp"

namespace sat {

struct Problem {
  i32 variable_count;
  i32 clause_count;

  u64 *clauses;
  u64 *negations;

  // TODO: include more bookkeeping structures
};

Problem init_problem(i32 variable_count, i32 clause_count);

void add_variable(Problem *problem, i32 clause_id, i32 variable_id, bool negate);

void dump_problem(Problem *problem);

} // namespace sat

#endif
