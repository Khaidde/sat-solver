#include "general.hpp"

#include "os.hpp"
#include "solver.hpp"

namespace sat {

struct Parser {
  i32 line;
  i32 idx;
  File file;
};

bool is_eof(Parser *parser) { return parser->idx >= parser->file.length; }

char at(Parser *parser) {
  assert(!is_eof(parser));
  return parser->file.data[parser->idx];
}

bool eat(Parser *parser) {
  assert(!is_eof(parser));

  if (at(parser) == '\n') ++parser->line;

  ++parser->idx;
  return is_eof(parser);
}

bool is_whitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

bool eat_whitespace(Parser *parser) {
  while (!is_eof(parser) && is_whitespace(at(parser))) eat(parser);
  return is_eof(parser);
}

i32 read_int(Parser *parser) {
  i32 number = 0;
  while (!is_eof(parser) && !is_whitespace(at(parser))) {
    char ch = at(parser);
    if ('0' > ch || ch > '9') panic("Found non-digit but expected number at line %d\n", parser->line);
    number = 10 * number + (ch - '0');
    eat(parser);
  }
  return number;
}

Result parse(Problem *problem, cstr input_path) {
  Parser parser;
  parser.line = 1;
  parser.idx  = 0;
  parser.file = read_file(input_path);
  if (!parser.file.data) return err;

  if (eat_whitespace(&parser)) panic("File is empty\n");

  bool has_problem_line = false;
  i32 variable_count;
  i32 clause_count;
  while (!is_eof(&parser)) {
    if (at(&parser) == 'c') {
      while (!is_eof(&parser) && at(&parser) != '\n') eat(&parser);

      if (is_eof(&parser)) panic("Expected problem line after comment\n");

      eat_whitespace(&parser);
    } else if (at(&parser) == 'p') {
      eat(&parser); // eat p

      if (eat_whitespace(&parser)) panic("Expected 'FORMAT' section after 'p'\n");

      // Eat 'FORMAT' section of problem line
      while (!is_eof(&parser) && !is_whitespace(at(&parser))) eat(&parser);

      if (eat_whitespace(&parser)) panic("Expected 'VARIABLE_COUNT' section after 'FORMAT' section\n");

      variable_count = read_int(&parser);

      if (eat_whitespace(&parser)) panic("Expected 'CLAUSE_COUNT' section after 'VARIABLE_COUNT' section\n");

      clause_count = read_int(&parser);

      eat_whitespace(&parser);

      has_problem_line = true;

      break;
    } else {
      panic("Expected 'c' or 'p' but found character '%c' at start of line %d\n", at(&parser), parser.line);
    }
  }

  if (!has_problem_line) panic("Expected a problem line in the preamble starting with 'p'\n");
  if (variable_count <= 0) panic("Problem must have more than 0 variables\n");
  if (clause_count <= 0) panic("Problem must have more than 0 clauses\n");

  *problem = init_problem(variable_count, clause_count);
  printf("CNF Problem: %d variables, %d clauses\n", variable_count, clause_count);

  i32 variable_count_in_clause = 0;
  i32 one_variable_id;
  bool one_variable_value;
  i32 clause_id = 0;
  debug("clause%d: ", clause_id);
  while (!is_eof(&parser)) {
    char ch = at(&parser);
    assert(!is_whitespace(ch));

    bool is_negated = false;
    if (ch == '-') {
      if (eat(&parser)) panic("Expected number after '-' on line %d\n", parser.line);
      is_negated = true;
    }

    if (ch == '%') break;

    i32 variable_id = read_int(&parser);
    if (variable_id) {
      debug(is_negated ? "-" : " ");
      debug("%-3d ", variable_id);

      add_variable(problem, clause_id, variable_id, is_negated);
      ++variable_count_in_clause;

      one_variable_id    = variable_id;
      one_variable_value = !is_negated;
    } else {
      if (variable_count_in_clause == 0) panic("Empty clause at line %d\n", parser.line);

      if (variable_count_in_clause == 1) {
        set_variable(problem, one_variable_id, one_variable_value);
        debug("\t\t// Optimize 1-literal x%d to %d", one_variable_id, one_variable_value);
      }
      debug("\n");
      ++clause_id;
      debug("clause%d: ", clause_id);
      variable_count_in_clause = 0;
    }

    eat_whitespace(&parser);
  }

  if (variable_count_in_clause > 0) {
    debug("\n");
    ++clause_id;
  }

  if (clause_count != clause_id) panic("Clause count of %d does not match actual %d\n", clause_count, clause_id);

  return ok;
}

Result solve(cstr input_path) {
  Problem problem;
  if (parse(&problem, input_path)) return err;

  // TODO: delete
  // dump_problem(&problem);

  if (dpll_solve(&problem) == SAT) {
    // print_sat_solution(&problem);
    return ok;
  } else {
    printf("UNSAT\n");
    return err;
  }
}

} // namespace sat

i32 main(i32 argc, char **argv) {
  if (argc != 2) {
    error("Expected usage: sat [input].cnf\n");
    return err;
  }

  if (sat::solve(argv[1])) return err;

  return ok;
}

