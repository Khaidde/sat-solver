#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "general.hpp"

namespace sat {

struct SimpleWatchlistNode {
  i32 clause_id;
  SimpleWatchlistNode *next;
};

struct Problem {
  i32 variable_count;
  i32 clause_count;

  u64 *clauses;
  u64 *negations;

  u64 *unassigned;
  u64 *assigned_values;

  i32 decision_stack_size;
  i32 *decision_stack;
  u64 **previous_unassigned_stack;

  i32 propagation_stack_size;
  i32 *propagation_stack;

  SimpleWatchlistNode **variable_to_clause;
};

Problem init_problem(i32 variable_count, i32 clause_count);

void add_variable(Problem *problem, i32 clause_id, i32 variable_id, bool negate);

void set_variable(Problem *problem, i32 variable_id, bool value);

enum ProblemResult {
  SAT,
  UNSAT,
};

ProblemResult dpll_solve(Problem *problem);

void print_sat_solution(Problem *problem);

void dump_problem(Problem *problem);

} // namespace sat

#endif
